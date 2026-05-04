// scan_result_view_model.cpp
#include "scan_result_view_model.h"
#include "scan_result_repository.h"
#include "scan_result_formatter.h"
#include "process_manager.h"
#include <QBrush>
#include <QColor>
#include <QString>

ScanResultViewModel::ScanResultViewModel(ScanResultRepository* repo, QObject* parent)
    : QAbstractTableModel(parent)
    , m_repo(repo)
{
    // repository 的生命周期由 ScanManager 管理，这里仅保存裸指针
}

void ScanResultViewModel::onRepositoryReplaced()
{
    beginResetModel();
    endResetModel();
}

void ScanResultViewModel::onDeltaApplied(int minRow, int maxRow)
{
    // 只通知 Value / Previous / First 列变化（地址列不更新）
    if (minRow <= maxRow) {
        emit dataChanged(index(minRow, 1), index(maxRow, 3), { Qt::DisplayRole });
    }
}

void ScanResultViewModel::setDisplayType(ScanDataType type)
{
    m_displayType = type;
    // 注意：调用方应在需要时手动触发 onRepositoryReplaced() 以刷新显示
}

ScanDataType ScanResultViewModel::displayType() const
{
    return m_displayType;
}

int ScanResultViewModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) return 0;
    if (!m_repo) return 0;
    return std::min(static_cast<int>(m_repo->resultCount()), MAX_DISPLAY);
}

int ScanResultViewModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid()) return 0;
    return 4;
}

QVariant ScanResultViewModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || !m_repo)
        return {};

    int row = index.row();
    if (row < 0 || row >= rowCount())
        return {};

    const ScanResult* item = m_repo->resultAt(row);
    if (!item)
        return {};

    // 地址列
    if (index.column() == 0) {
        if (role == Qt::DisplayRole) {
            std::string display;
            bool isBase = false;
            ProcessManager::instance().resolveAddress(item->address, display, isBase);
            return QString::fromStdString(display);
        }
        if (role == Qt::ForegroundRole) {
            std::string display;
            bool isBase = false;
            ProcessManager::instance().resolveAddress(item->address, display, isBase);
            if (isBase)
                return QBrush(QColor(0, 128, 0)); // 绿色标识模块基址
        }
        return {};
    }

    // 数值 / 字符串 / 字节数组列
    if (role == Qt::DisplayRole) {
        return formatCell(*item, index.column());
    }

    // 第1列（Value）变化时红色标记
    if (role == Qt::ForegroundRole && index.column() == 1 && item->changed) {
        return QBrush(Qt::red);
    }

    return {};
}

QVariant ScanResultViewModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
        return {};
    switch (section) {
    case 0: return QStringLiteral("Address");
    case 1: return QStringLiteral("Value");
    case 2: return QStringLiteral("Previous");
    case 3: return QStringLiteral("First");
    default: return {};
    }
}

QString ScanResultViewModel::formatCell(const ScanResult& item, int column) const
{
    // 字符串 / 字节数组类型：直接从当前内存读取显示（即使对于 Previous / First 列）
    // 注意：这与 CE 的行为可能不完全一致，但保持了原有的显示逻辑
    if (isStringType(m_displayType) || m_displayType == ScanDataType::ByteArray) {
        if (m_displayType == ScanDataType::ByteArray)
            return QString::fromStdString(ScanResultFormatter::formatByteArrayAt(item.address));
        else
            return QString::fromStdString(ScanResultFormatter::formatStringAt(item.address, m_displayType));
    }

    // 数值类型：根据列获取对应的值
    uint64_t raw = 0;
    switch (column) {
    case 1: raw = item.value;      break;   // Current Value
    case 2: raw = item.lastValue;  break;   // Previous Value
    case 3: raw = item.firstValue; break;   // First Scan Value
    default: return {};
    }
    return QString::fromStdString(ScanResultFormatter::formatValue(raw, m_displayType));
}

uint64_t ScanResultViewModel::getAddress(int row) const
{
    return m_repo ? m_repo->addressAtIndex(row) : 0;
}