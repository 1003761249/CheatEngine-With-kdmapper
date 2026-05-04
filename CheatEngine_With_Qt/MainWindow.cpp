// mainwindow.cpp
#include "mainwindow.h"
#include "ui_CheatEngine_With_Qt.h"
#include "address_list_model.h"
#include "thread_pool.h"
#include "process_dialog.h"
#include "process_manager.h"
#include "scan_service.h"
#include "scan_result_view_model.h"
#include "scan_request.h"

#include <QTimer>
#include <QTableView>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QApplication>
#include <QStatusBar>
#include <QStringList>
#include <cstring>

// ==================== 构造与析构 ====================
MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(std::make_unique<Ui::CheatEngine_With_QtClass>())
{
    setupUi();
    initServices();
    initViews();
    initTimers();
    connectSignals();
    applyDefaultState();
}

MainWindow::~MainWindow() = default;

// ==================== UI 基础 ====================
void MainWindow::setupUi() { ui->setupUi(this); }

void MainWindow::initServices()
{
    m_scanService = new ScanService(this);
    m_resultModel = m_scanService->resultModel();   // 获取 ViewModel 指针
    addressModel = new AddressListModel(this);
}

// 辅助函数：解析数值（支持十六进制）
uint64_t MainWindow::resolveValueInput(const QString& text, bool isHex) const {
    bool ok;
    return isHex ? text.toULongLong(&ok, 16) : text.toULongLong(&ok, 10);
}


void MainWindow::initViews()
{
    setupScanResultView();
    replaceAddressTable();
}

void MainWindow::setupScanResultView()
{
    QWidget* firstTab = ui->tabWidget->widget(0);
    QVBoxLayout* tabLayout = new QVBoxLayout(firstTab);
    tabLayout->setContentsMargins(0, 0, 0, 0);

    scanResultView = new QTableView(firstTab);
    scanResultView->setModel(m_resultModel);
    scanResultView->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    scanResultView->verticalHeader()->setDefaultSectionSize(24);
    scanResultView->horizontalHeader()->setStretchLastSection(false);
    scanResultView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
    scanResultView->setColumnWidth(0, 150);
    scanResultView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Interactive);
    scanResultView->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    scanResultView->setSelectionBehavior(QAbstractItemView::SelectRows);
    tabLayout->addWidget(scanResultView);
}

void MainWindow::replaceAddressTable()
{
    QLayout* addrLayout = ui->tableWidget_addressList->parentWidget()->layout();
    if (auto* vbox = qobject_cast<QVBoxLayout*>(addrLayout)) {
        vbox->removeWidget(ui->tableWidget_addressList);
        delete ui->tableWidget_addressList;
        addressView = new QTableView(this);
        addressView->setModel(addressModel);
        addressView->setSelectionBehavior(QAbstractItemView::SelectRows);
        addressView->setSelectionMode(QAbstractItemView::SingleSelection);
        vbox->addWidget(addressView);
    }
}

// ==================== 定时器 ====================
void MainWindow::initTimers()
{
    freezeTimer = new QTimer(this);
    connect(freezeTimer, &QTimer::timeout, this, [this]() {
        auto& items = addressModel->items();
        auto mem = ProcessManager::instance().memory();
        if (!mem) return;
        for (auto& item : items) {
            if (item.frozen)
                mem->write(item.address, &item.value, sizeof(item.value));
        }
        });
    freezeTimer->start(500);

    healthTimer = new QTimer(this);
    connect(healthTimer, &QTimer::timeout, this, [this]() {
        if (m_attachedToProcess && !ProcessManager::instance().isProcessAlive()) {
            m_attachedToProcess = false;
            QMetaObject::invokeMethod(this, [this] { onProcessTerminated(); });
        }
        });
    healthTimer->start(1000);
}

// ==================== 信号连接 ====================
void MainWindow::connectSignals()
{
    connect(ui->pushButton_openProcess, &QPushButton::clicked,
        this, &MainWindow::onOpenProcess);
    connect(ui->pushButton_new_find, &QPushButton::clicked,
        this, &MainWindow::onFirstScan);
    connect(ui->pushButton_next_find, &QPushButton::clicked,
        this, &MainWindow::onNextScan);

    connect(m_scanService, &ScanService::scanCompleted,
        this, &MainWindow::onScanCompleted);
    connect(m_scanService, &ScanService::progressChanged,
        this, &MainWindow::onProgressChanged);

    connect(scanResultView, &QTableView::doubleClicked,
        this, &MainWindow::onDoubleClickScanResult);

    connect(ui->pushButton_delete_all_address, &QPushButton::clicked, this, [this]() {
        addressModel->clear();
        });

    connect(ui->comboBox_atribute_For_Find, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, &MainWindow::updateScanTypeUi);
    updateScanTypeUi(ui->comboBox_atribute_For_Find->currentIndex());
}

// ==================== UI 状态管理 ====================
void MainWindow::updateScanTypeUi(int index)
{
    if (index < 0) return;

    // 定义需要显示的控件状态
    bool showInput1 = true;
    bool showInput2 = false;
    bool showHex = true;

    if (m_isFirstScan) {
        // 首次扫描阶段的索引逻辑: 0:精确, 1:大于, 2:小于, 3:介于两者, 4:未知
        switch (index) {
        case 3: // 介于两者之间 (Between)
            showInput1 = true;
            showInput2 = true;
            break;
        case 4: // 未知初始值 (Unknown)
            showInput1 = false;
            showHex = false;
            break;
        default: // 精确/大于/小于
            showInput1 = true;
            showInput2 = false;
            break;
        }
    }
    else {
        // 再次扫描阶段的索引逻辑 (对应 onScanCompleted 中 addItems 的顺序)
        // {"精确数值", "增加的值", "减少的值", "变动的值", "未变动的值", "介于两者之间"}
        switch (index) {
        case 0: // 精确数值
            showInput1 = true;
            showInput2 = false;
            break;
        case 5: // 介于两者之间 (根据你 addItems 的顺序，这是第6项)
            showInput1 = true;
            showInput2 = true;
            break;
        default: // 增加/减少/变动/未变动
            showInput1 = false;
            showInput2 = false;
            showHex = false;
            break;
        }
    }

    // 应用可见性
    ui->lineEdit_ValueInput->setVisible(showInput1);
    ui->checkBox_Hex_Value->setVisible(showInput1 && showHex);

    ui->label_and->setVisible(showInput2);
    ui->lineEdit_ValueInput2->setVisible(showInput2);
}

void MainWindow::applyDefaultState()
{
    ui->pushButton_new_find->setEnabled(false);
    ui->pushButton_next_find->setEnabled(false);
}

// ==================== 打开进程 ====================
void MainWindow::onOpenProcess()
{
    ProcessDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted) return;

    auto p = dlg.selectedProcess();

    m_scanService->cancel();
    m_scanService->stopAutoRefresh();
    m_scanService->clear();           // 使用 ScanService::clear()

    ProcessManager::instance().detach();

    if (ProcessManager::instance().attach(p)) {
        m_attachedToProcess = true;
        m_isFirstScan = true;

        ui->label_Process_name->setText(QString::fromStdString(p.name));
        setWindowTitle(QString("Cheat Engine - %1").arg(QString::fromStdString(p.name)));

        QComboBox* moduleBox = ui->comboBox_process_module_List;
        moduleBox->clear();
        moduleBox->addItem("All");
        const auto& modules = ProcessManager::instance().modules();
        for (const auto& mod : modules)
            moduleBox->addItem(QString::fromStdString(mod.name));

        ui->pushButton_new_find->setEnabled(true);
        ui->pushButton_next_find->setEnabled(false);
    }
    else {
        QMessageBox::warning(this, "Error", "Failed to attach to process.");
    }
}

// ==================== 构建扫描请求（保留原解析逻辑） ====================
ScanDataType MainWindow::parseDataTypeFromUI() const
{
    int idx = ui->comboBox_Value_Data_Size->currentIndex();
    switch (idx) {
    case 0: return ScanDataType::Int8;
    case 1: return ScanDataType::Int16;
    case 2: return ScanDataType::Int32;
    case 3: return ScanDataType::Int64;
    case 4: return ScanDataType::Float32;
    case 5: return ScanDataType::Float64;
    case 6: return ui->checkBox_use_UTF16->isChecked() ? ScanDataType::Utf16String : ScanDataType::AsciiString;
    case 7: return ScanDataType::ByteArray;
    default: return ScanDataType::Int64;
    }
}

ValueParams MainWindow::parseValueParams(ScanType type, ScanDataType dataType) const
{
    ValueParams vp;
    if (type == ScanType::UnknownInitial) return vp;

    QString text1 = ui->lineEdit_ValueInput->text();
    bool isHex = ui->checkBox_Hex_Value->isChecked();

    if (isFloatingPoint(dataType)) {
        bool ok = false;
        double d1 = text1.toDouble(&ok);
        if (!ok && !text1.isEmpty()) return {};
        if (dataType == ScanDataType::Float32) {
            float f = static_cast<float>(d1);
            std::memcpy(&vp.value1, &f, sizeof(f));
        }
        else {
            std::memcpy(&vp.value1, &d1, sizeof(d1));
        }

        if (type == ScanType::Between) {
            QString text2 = ui->lineEdit_ValueInput2->text();
            double d2 = text2.toDouble(&ok);
            if (!ok) return {};
            if (dataType == ScanDataType::Float32) {
                float f2 = static_cast<float>(d2);
                std::memcpy(&vp.value2, &f2, sizeof(f2));
            }
            else {
                std::memcpy(&vp.value2, &d2, sizeof(d2));
            }
        }
    }
    else {
        bool ok = false;
        vp.value1 = isHex ? text1.toULongLong(&ok, 16) : text1.toULongLong(&ok, 10);
        if (!ok && !text1.isEmpty()) return {};
        if (type == ScanType::Between) {
            QString text2 = ui->lineEdit_ValueInput2->text();
            bool ok2 = false;
            vp.value2 = isHex ? text2.toULongLong(&ok2, 16) : text2.toULongLong(&ok2, 10);
            if (!ok2) return {};
        }
    }
    return vp;
}

StringParams MainWindow::parseStringParams() const
{
    StringParams sp;
    sp.caseSensitive = ui->checkBox_Caps_Check->isChecked();
    QString text = ui->lineEdit_ValueInput->text();
    if (parseDataTypeFromUI() == ScanDataType::Utf16String) {
        auto utf16 = text.utf16();
        sp.text.assign(reinterpret_cast<const char*>(utf16), text.size() * 2);
    }
    else {
        sp.text = text.toStdString();
    }
    return sp;
}

AobParams MainWindow::parseAobParams() const
{
    AobParams ap;
    QString text = ui->lineEdit_ValueInput->text().trimmed();
    QStringList tokens = text.split(' ', Qt::SkipEmptyParts);
    for (const QString& tok : tokens) {
        if (tok.compare("??", Qt::CaseInsensitive) == 0 || tok == "?") {
            ap.pattern.push_back(0);
            ap.mask.push_back(false);
        }
        else {
            bool ok = false;
            uint32_t val = tok.toUInt(&ok, 16);
            if (!ok || val > 0xFF) return {};
            ap.pattern.push_back(static_cast<uint8_t>(val));
            ap.mask.push_back(true);
        }
    }
    return ap;
}

ScanRequest MainWindow::buildFirstScanRequest() const
{
    ScanRequest req;
    req.mode = ScanMode::First;
    req.dataType = parseDataTypeFromUI();

    // 1. 映射扫描类型 (First Scan 专属)
    // 对应 UI comboBox_atribute_For_Find: 0:精确数值, 1:值大于, 2:值小于, 3:介于两者, 4:未知初始值
    int scanTypeIdx = ui->comboBox_atribute_For_Find->currentIndex();
    static const ScanType firstTypeMap[] = {
        ScanType::ExactValue,   // 0
        ScanType::GreaterThan,  // 1
        ScanType::LessThan,     // 2
        ScanType::Between,       // 3
        ScanType::UnknownInitial // 4
    };
    req.firstType = (scanTypeIdx >= 0 && scanTypeIdx <= 4) ? firstTypeMap[scanTypeIdx] : ScanType::ExactValue;

    // 2. 内存对齐 (Alignment) - 核心 CE 逻辑
    // CE "Fast Scan" 勾选时按指定步长对齐（通常是4）；不勾选时 alignment=1，即逐字节扫描全内存
    if (ui->checkBox_fast_scan->isChecked()) {
        bool ok = false;
        uint32_t align = ui->lineEdit_fast_scan_value->text().toUInt(&ok);
        req.alignment = (ok && align > 0) ? align : 4;
    }
    else {
        req.alignment = 1;
    }

    // 3. 模块范围过滤
    // 如果下拉框不是 "All"，则将扫描范围锁定在特定模块的基址和大小内
    QString modName = ui->comboBox_process_module_List->currentText();
    if (modName != "All") {
        const auto* mod = ProcessManager::instance().getModuleByName(modName.toStdString());
        if (mod) {
            req.moduleBase = mod->base;
            req.moduleSize = mod->size;
        }
    }


    // 5. 核心参数解析：确保 isHex 变量真正发挥作用
    bool isHex = ui->checkBox_Hex_Value->isChecked();

    if (isStringType(req.dataType)) {
        // 字符串扫描：调用专项解析器（区分大小写/编码）
        req.params = parseStringParams();
    }
    else if (req.dataType == ScanDataType::ByteArray) {
        // AOB 扫描：调用专项解析器（处理通配符 ??）
        req.params = parseAobParams();
    }
    else {
        // 数值类型：集成解析逻辑，直接处理 isHex
        ValueParams vp;
        if (req.firstType != ScanType::UnknownInitial) {
            QString text1 = ui->lineEdit_ValueInput->text();
            bool ok1 = false;

            if (isFloatingPoint(req.dataType)) {
                // 浮点数解析（浮点数不涉及十六进制输入）
                double d1 = text1.toDouble(&ok1);
                if (ok1) {
                    if (req.dataType == ScanDataType::Float32) {
                        float f = static_cast<float>(d1);
                        std::memcpy(&vp.value1, &f, 4);
                    }
                    else {
                        std::memcpy(&vp.value1, &d1, 8);
                    }
                }
            }
            else {
                // 整数解析：关键点！根据 isHex 切换进制
                vp.value1 = isHex ? text1.toULongLong(&ok1, 16) : text1.toULongLong(&ok1, 10);
            }

            // 处理“介于两者之间” (Between) 的第二个值
            if (req.firstType == ScanType::Between) {
                QString text2 = ui->lineEdit_ValueInput2->text();
                bool ok2 = false;
                if (isFloatingPoint(req.dataType)) {
                    double d2 = text2.toDouble(&ok2);
                    if (ok2) {
                        if (req.dataType == ScanDataType::Float32) {
                            float f2 = static_cast<float>(d2);
                            std::memcpy(&vp.value2, &f2, 4);
                        }
                        else {
                            std::memcpy(&vp.value2, &d2, 8);
                        }
                    }
                }
                else {
                    vp.value2 = isHex ? text2.toULongLong(&ok2, 16) : text2.toULongLong(&ok2, 10);
                }
            }
        }
        req.params = vp;
    }

    return req;
}

ScanRequest MainWindow::buildNextScanRequest() const
{
    ScanRequest req;
    req.mode = ScanMode::Next;
    req.dataType = m_currentDataType;
    req.firstType = m_currentFirstScanType;

    int nextIdx = ui->comboBox_atribute_For_Find->currentIndex();
    switch (nextIdx) {
        case 0: req.nextType = NextScanType::Equal; break;
        case 1: req.nextType = NextScanType::Increased; break;
        case 2: req.nextType = NextScanType::Decreased; break;
        case 3: req.nextType = NextScanType::Changed; break;
        case 4: req.nextType = NextScanType::Unchanged; break;
        case 5: req.nextType = NextScanType::Between; break;
        default: req.nextType = NextScanType::Equal; break;
    }
    ValueParams vp;
    QString text1 = ui->lineEdit_ValueInput->text();
    if (isFloatingPoint(req.dataType)) {
        double d = text1.toDouble();
        if (req.dataType == ScanDataType::Float32) {
            float f = static_cast<float>(d);
            std::memcpy(&vp.value1, &f, sizeof(f));
        }
        else {
            std::memcpy(&vp.value1, &d, sizeof(d));
        }
    }
    if (req.nextType == NextScanType::Equal || req.nextType == NextScanType::Between) {
        vp = parseValueParams(static_cast<ScanType>(req.nextType), req.dataType);
    }
    else {
        vp.value1 = text1.toULongLong(nullptr, 10);
    }
    req.params = vp;
    return req;
}

// ==================== 扫描执行 ====================
void MainWindow::onFirstScan()
{
    if (!ProcessManager::instance().memory()) {
        QMessageBox::warning(this, "Error", "Please open a process first.");
        return;
    }
    if (m_scanService->isScanning()) return;

    ScanRequest req = buildFirstScanRequest();
    // 参数校验
    if (std::holds_alternative<StringParams>(req.params)) {
        if (std::get<StringParams>(req.params).text.empty()) {
            QMessageBox::warning(this, "Error", "Please enter a string to search.");
            return;
        }
    }
    else if (std::holds_alternative<AobParams>(req.params)) {
        if (std::get<AobParams>(req.params).pattern.empty()) {
            QMessageBox::warning(this, "Error", "Please enter a byte pattern.");
            return;
        }
    }
    else if (std::holds_alternative<ValueParams>(req.params)) {
        if (req.firstType != ScanType::UnknownInitial &&
            std::get<ValueParams>(req.params).value1 == 0 &&
            ui->lineEdit_ValueInput->text().isEmpty()) {
            QMessageBox::warning(this, "Error", "Invalid value.");
            return;
        }
    }

    
    m_currentDataType = req.dataType;
    m_currentFirstScanType = req.firstType;
    m_isFirstScan = false;

    m_scanService->startScan(req);
    ui->pushButton_new_find->setEnabled(false);
    ui->pushButton_next_find->setEnabled(false);
}

void MainWindow::onNextScan()
{
    if (!ProcessManager::instance().memory()) {
        QMessageBox::warning(this, "Error", "No process attached.");
        return;
    }
    if (m_scanService->isScanning()) return;
    if (!m_scanService->hasResults()) {
        QMessageBox::warning(this, "Error", "Please perform a first scan first.");
        return;
    }

    ScanRequest req = buildNextScanRequest();
    if (std::holds_alternative<ValueParams>(req.params)) {
        if (std::get<ValueParams>(req.params).value1 == 0 &&
            ui->lineEdit_ValueInput->text().isEmpty()) {
            QMessageBox::warning(this, "Error", "Invalid value.");
            return;
        }
    }

    m_scanService->startScan(req);
    ui->pushButton_new_find->setEnabled(false);
    ui->pushButton_next_find->setEnabled(false);
}

// ==================== 扫描完成槽 ====================
void MainWindow::onScanCompleted()
{

    bool canNextScan = m_scanService->hasResults() ||
        (m_currentFirstScanType == ScanType::UnknownInitial && m_scanService->hasSnapshot());

    ui->pushButton_new_find->setEnabled(true);
    ui->pushButton_next_find->setEnabled(canNextScan);


    ui->comboBox_atribute_For_Find->blockSignals(true);
    ui->comboBox_atribute_For_Find->clear();

    if (m_isFirstScan) {
        ui->comboBox_atribute_For_Find->addItems({ "精确数值", "值大于", "值小于", "介于两者之间", "未知初始值" });
    }
    else {
        ui->comboBox_atribute_For_Find->addItems({ "精确数值", "增加的值", "减少的值", "变动的值", "未变动的值", "介于两者之间" });
    }

    ui->comboBox_atribute_For_Find->setCurrentIndex(0);
    ui->comboBox_atribute_For_Find->blockSignals(false);

    // 【关键修复】手动调用一次更新函数，因为 signals 被 block 了
    updateScanTypeUi(ui->comboBox_atribute_For_Find->currentIndex());

    if (isStringType(m_currentDataType) || m_currentDataType == ScanDataType::ByteArray) {
        m_scanService->stopAutoRefresh();
    }
    else {
        m_scanService->startAutoRefresh(200);
    }

    ui->progressBar->setVisible(false);
}

// ==================== 进度变化槽 ====================
void MainWindow::onProgressChanged(int completed, int total)
{
    ui->progressBar->setVisible(true);
    ui->progressBar->setRange(0, total);
    ui->progressBar->setValue(completed);
    statusBar()->showMessage(
        QString("Scanning... region %1 / %2").arg(completed).arg(total));
}

// ==================== 双击添加地址 ====================
void MainWindow::onDoubleClickScanResult(const QModelIndex& index)
{
    if (!ProcessManager::instance().memory()) return;

    // 获取地址：需要调用 ScanResultViewModel::getAddress(int row)
    // 如果该接口尚未实现，请在 ScanResultViewModel 中添加：
    //   uint64_t getAddress(int row) const { return m_repo ? m_repo->addressAtIndex(row) : 0; }
    uint64_t addr = m_resultModel->getAddress(index.row());
    if (addr == 0) return;

    uint64_t val = 0;
    ProcessManager::instance().memory()->read(addr, &val, sizeof(val));
    QString desc = QString("Address 0x%1").arg(addr, 0, 16);
    addressModel->addItem(addr, desc, val);
}

// ==================== 进程退出与重置 ====================
void MainWindow::resetToNoProcess()
{
    m_attachedToProcess = false;
    m_isFirstScan = true;

    m_scanService->cancel();
    m_scanService->stopAutoRefresh();
    m_scanService->clear();                     // 使用 clear() 替代 clearResults()

    ProcessManager::instance().detach();
    addressModel->clear();

    ui->pushButton_new_find->setEnabled(false);
    ui->pushButton_next_find->setEnabled(false);
    ui->label_Process_name->setText("请选择进程");
    setWindowTitle("Cheat Engine");
    ui->comboBox_process_module_List->clear();
    ui->comboBox_process_module_List->addItem("All");
    ui->progressBar->setVisible(false);
    statusBar()->clearMessage();
}

void MainWindow::onProcessTerminated()
{
    m_attachedToProcess = false;
    QMessageBox::information(this, "Process terminated", "The target process has exited.");
    resetToNoProcess();
}