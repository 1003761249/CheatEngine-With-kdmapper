#pragma once

#include <QObject>
#include <QTimer>
#include <atomic>
#include <memory>
#include "scan_request.h"
#include "scan_engine.h"

class ScanEngine;
class ScanResultRepository;
class ScanResultViewModel;

/// @brief 扫描服务门面，封装引擎调度、结果存储、自动刷新。
///        上层只通过此类控制整个扫描流程。
class ScanService : public QObject
{
    Q_OBJECT
public:
    explicit ScanService(QObject* parent = nullptr);
    ~ScanService() override;

    // ------------------ 扫描控制 ------------------
    /// 启动异步扫描（重复调用在扫描进行中会被忽略）
    void startScan(const ScanRequest& request);
    /// 取消当前扫描并停止自动刷新
    void cancel();

    // ------------------ 状态查询 ------------------
    bool isScanning() const;
    bool hasResults() const;          ///< 仓库中是否有结果
    int  totalResults() const;        ///< 仓库中总结果数
    bool hasSnapshot() const;         //   仓库保存了未知初始值的快照


    // ------------------ 结果视图 ------------------
    ScanResultViewModel* resultModel() const;

    // ------------------ 自动刷新 ------------------
    void startAutoRefresh(int intervalMs = 200);
    void stopAutoRefresh();
    bool isAutoRefreshing() const;

    // ------------------ 管理 ------------------
    void clear();                     ///< 清除所有结果并停止刷新
    void reset();                     ///< 完全重置（引擎+仓库+视图）

signals:
    /// 扫描完成（结果已存入仓库并更新视图）
    void scanCompleted();
    /// 进度信息（completed, total），仅扫描期间发射
    void progressChanged(int completed, int total);

private slots:
    void onRefreshTimer();

private:
    void onScanFinished(ScanEngine::ResultPack pack);
    void performRefresh();


    std::unique_ptr<ScanEngine>            m_engine;
    std::unique_ptr<ScanResultRepository>  m_repository;
    ScanResultViewModel* m_viewModel;

    QTimer* m_refreshTimer;
    QTimer* m_progressTimer = nullptr;     // 动态创建的进度定时器

    std::atomic<bool> m_scanning{ false };
    std::atomic<bool> m_refreshing{ false };
    std::atomic<bool> m_cancelling{ false }; // 防止取消期间进度信号干扰
    int m_currentGeneration = 0;
};