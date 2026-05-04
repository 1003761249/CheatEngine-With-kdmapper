// ScanResultViewModel – Qt 显示层
//职责：包装 ScanResultRepository* ，实现 QAbstractTableModel。

#pragma once
#include <QAbstractTableModel>
#include "scan_types.h"
#include "scan_result.h"

class ScanResultRepository;

/// @brief 为 QTableView 提供数据的适配器。
///        只依赖 Repository，不处理数据存储或刷新。
class ScanResultViewModel : public QAbstractTableModel {
    Q_OBJECT
public:
    explicit ScanResultViewModel(ScanResultRepository* repo, QObject* parent = nullptr);

    // 通知 View 数据已整体变更
    void onRepositoryReplaced();
    // 通知 View 部分行更新
    void onDeltaApplied(int minRow, int maxRow);

    void setDisplayType(ScanDataType type);
    ScanDataType displayType() const;

    // QAbstractTableModel 接口
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section, Qt::Orientation, int role) const override;
    uint64_t getAddress(int row) const;


private:
    ScanResultRepository* m_repo;   // 不拥有所有权
    ScanDataType m_displayType = ScanDataType::Int64;
    static constexpr int MAX_DISPLAY = 50000;

    QString formatCell(const ScanResult& item, int column) const;
};