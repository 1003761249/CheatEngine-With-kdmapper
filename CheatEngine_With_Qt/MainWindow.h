#pragma once
#include <QMainWindow>
#include <QTimer>
#include <QTableView>
#include <atomic>
#include <memory>
#include "scan_request.h"

namespace Ui {
    class CheatEngine_With_QtClass;
}

class ScanService;
class ScanResultViewModel;
class AddressListModel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private:
    void setupUi();
    void initServices();          // 创建 ScanService 并获取视图模型
    void initViews();
    void setupScanResultView();
    void replaceAddressTable();
    void initTimers();
    void connectSignals();
    void updateScanTypeUi(int index);
    void applyDefaultState();

    // 扫描流程
    ScanRequest buildFirstScanRequest() const;
    ScanRequest buildNextScanRequest() const;
    ScanDataType parseDataTypeFromUI() const;
    ValueParams parseValueParams(ScanType type, ScanDataType dataType) const;
    StringParams parseStringParams() const;
    AobParams parseAobParams() const;

    void resetToNoProcess();
    void onProcessTerminated();


    uint64_t resolveValueInput(const QString& text, bool isHex) const;


    // 状态
    std::unique_ptr<Ui::CheatEngine_With_QtClass> ui;
    ScanService* m_scanService = nullptr;
    ScanResultViewModel* m_resultModel = nullptr;
    AddressListModel* addressModel = nullptr;

    QTableView* scanResultView = nullptr;
    QTableView* addressView = nullptr;

    QTimer* freezeTimer = nullptr;
    QTimer* healthTimer = nullptr;

    bool m_attachedToProcess = false;
    bool m_isFirstScan = true;
    ScanDataType m_currentDataType = ScanDataType::Int64; // 记录当前数据类型
    ScanType     m_currentFirstScanType = ScanType::ExactValue;

private slots:
    void onOpenProcess();
    void onFirstScan();
    void onNextScan();
    void onDoubleClickScanResult(const QModelIndex& index);
    void onScanCompleted();
    void onProgressChanged(int completed, int total);
};