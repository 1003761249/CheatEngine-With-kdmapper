// scan_service.cpp
#include "scan_service.h"
#include "scan_result_repository.h"
#include "scan_result_view_model.h"
#include "thread_pool.h"
#include "process_manager.h"
#include <QMetaObject>

ScanService::ScanService(QObject* parent)
    : QObject(parent)
    , m_engine(std::make_unique<ScanEngine>())
    , m_repository(std::make_unique<ScanResultRepository>())
    , m_refreshTimer(new QTimer(this))
{
    m_viewModel = new ScanResultViewModel(m_repository.get(), this);
    connect(m_refreshTimer, &QTimer::timeout, this, &ScanService::onRefreshTimer);
}

ScanService::~ScanService() = default;

// ---------------- 公有接口 ----------------

void ScanService::startScan(const ScanRequest& request)
{
    if (m_scanning.exchange(true))
        return;   // 已有扫描在进行

    // 开始新扫描前停止自动刷新（扫描期间数据可能变更）
    stopAutoRefresh();
    m_cancelling = false;

    // 创建进度查询定时器，在扫描期间定期发射进度
    m_progressTimer = new QTimer(this);
    connect(m_progressTimer, &QTimer::timeout, this, [this]() {
        if (!m_scanning.load() || m_cancelling.load()) {
            // 扫描结束或取消，定时器自毁
            if (m_progressTimer) {
                m_progressTimer->stop();
                m_progressTimer->deleteLater();
                m_progressTimer = nullptr;
            }
            return;
        }
        emit progressChanged(m_engine->regionsCompleted(),
            m_engine->totalRegions());
        });
    m_progressTimer->start(50);   // 20Hz 进度更新


    // 构造请求副本，为再次扫描注入前次快照
    ScanRequest finalRequest = request;
    if (finalRequest.mode == ScanMode::Next) {
        finalRequest.prevResults = m_repository->createSnapshot();

        // 前提是 UI 传来的 request.firstType 正确带入了上一次的扫描类型
        bool isFirstNextAfterUnknown = (finalRequest.firstType == ScanType::UnknownInitial &&
            (m_engine->hasSnapshot()|| !finalRequest.prevResults || finalRequest.prevResults->empty()));

        // 如果既不是未知初始值的后续扫描，仓库又是空的，那才判定为无效扫描并结束
        if (!isFirstNextAfterUnknown && (!finalRequest.prevResults || finalRequest.prevResults->empty())) {
            m_scanning.store(false);
            emit scanCompleted();
            return;
        }
    }

    GlobalThreadPool::instance().enqueue([this, finalRequest]() {
        auto pack = m_engine->execute(finalRequest);
        QMetaObject::invokeMethod(this, [this, pack = std::move(pack)]() mutable {
            onScanFinished(std::move(pack));
            });
        });
}

void ScanService::cancel()
{
    m_cancelling = true;
    m_engine->cancel();
    if (m_scanning.load()) {
        // 确保进度定时器停止
        if (m_progressTimer) {
            m_progressTimer->stop();
            m_progressTimer->deleteLater();
            m_progressTimer = nullptr;
        }
        m_scanning = false;
    }
    stopAutoRefresh();
}

bool ScanService::isScanning() const
{
    return m_scanning.load();
}

bool ScanService::hasResults() const
{
    
         
    return m_repository->resultCount() > 0 || this->hasSnapshot();
}

int ScanService::totalResults() const
{
    return static_cast<int>(m_repository->resultCount());
}

ScanResultViewModel* ScanService::resultModel() const
{
    return m_viewModel;
}

void ScanService::startAutoRefresh(int intervalMs)
{
    if (!m_scanning.load())
        m_refreshTimer->start(intervalMs);
}

void ScanService::stopAutoRefresh()
{
    m_refreshTimer->stop();
}

bool ScanService::isAutoRefreshing() const
{
    return m_refreshTimer->isActive();
}

void ScanService::clear()
{
    stopAutoRefresh();
    m_repository->replaceAllResults({});
    m_currentGeneration = m_repository->currentGeneration();
    m_viewModel->onRepositoryReplaced();
}

void ScanService::reset()
{
    clear();
    m_engine->cancel();
    m_scanning = false;
    // 如果正在扫描，等待线程结束？简单处理：直接重置标志
}

// ---------------- 内部实现 ----------------

void ScanService::onScanFinished(ScanEngine::ResultPack pack)
{
    // 进度定时器自毁
    if (m_progressTimer) {
        m_progressTimer->stop();
        m_progressTimer->deleteLater();
        m_progressTimer = nullptr;
    }

    if (m_cancelling.load() || m_engine->isCancelled()) {
        m_scanning = false;
        m_cancelling = false;
        return;
    }

    std::vector<ScanResult> allResults;
    if (pack.results) {
        // 获取池中所有线程产生的总元素数量
        const size_t total = pack.results->total_size();
        allResults.reserve(total);

        // 利用 Pool 提供的聚合 readChunk 接口分页合并数据
        // 即使数据最终都在内存 vector 中，分块读取也能避免 readChunk 内部产生过大的临时容器
        constexpr size_t kReadBatch = 100'000;
        for (size_t offset = 0; offset < total; offset += kReadBatch) {
            size_t count = std::min(kReadBatch, total - offset);

            // Pool::readChunk 会自动根据偏移量跨多个子缓存文件/内存块提取数据
            auto chunk = pack.results->readChunk(offset, count);

            allResults.insert(allResults.end(),
                std::make_move_iterator(chunk.begin()),
                std::make_move_iterator(chunk.end()));
        }

        // 数据收集完毕后，调用 clear 销毁所有子缓存对象及其磁盘临时文件
        pack.results->clear();
    }


    // 替换仓库数据
    m_repository->replaceAllResults(std::move(allResults));
    m_currentGeneration = m_repository->currentGeneration();

    // 更新视图模型
    m_viewModel->setDisplayType(pack.dataType);
    m_viewModel->onRepositoryReplaced();

    m_scanning = false;
    m_cancelling = false;

    emit scanCompleted();
}

void ScanService::onRefreshTimer()
{
    if (m_scanning.load() || m_refreshing.exchange(true))
        return;

    performRefresh();
}

void ScanService::performRefresh()
{
    auto snap = m_repository->createSnapshot();
    if (!snap || snap->empty()) {
        m_refreshing = false;
        return;
    }

    int gen = m_currentGeneration;
    auto mem = ProcessManager::instance().memory(); // 依赖全局进程管理器，实际项目保留

    // 在后台线程比较内存变化
    GlobalThreadPool::instance().enqueue([this, snap, gen, mem]() {
        std::vector<int> rows;
        std::vector<uint64_t> vals;
        std::vector<uint8_t> flags;

        for (size_t i = 0; i < snap->size(); ++i) {
            const auto& item = (*snap)[i];
            uint64_t newVal = 0;
            if (!mem->read(item.address, &newVal, sizeof(newVal)))
                continue;
            if (newVal != item.value) {
                rows.push_back(static_cast<int>(i));
                vals.push_back(newVal);
                flags.push_back(1);
            }
        }

        QMetaObject::invokeMethod(this, [this, gen, rows = std::move(rows),
            vals = std::move(vals), flags = std::move(flags)]() {
                if (gen == m_currentGeneration && !m_scanning.load()) {
                    m_repository->applyIncrementalUpdates(gen, rows, vals, flags);
                    if (!rows.empty()) {
                        int minRow = rows.front();
                        int maxRow = rows.back();
                        int maxDisplay = m_viewModel->rowCount() - 1;
                        maxRow = std::min(maxRow, maxDisplay);
                        if (minRow <= maxRow)
                            m_viewModel->onDeltaApplied(minRow, maxRow);
                    }
                }
                m_refreshing = false;
            });
        });
}

bool ScanService::hasSnapshot() const {
    return m_engine && m_engine->hasSnapshot();
}