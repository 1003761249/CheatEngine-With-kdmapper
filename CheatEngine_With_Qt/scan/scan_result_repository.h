#pragma once
#include <vector>
#include <atomic>
#include <mutex>
#include <memory>
#include "scan_result.h"

/// @brief 线程安全的结果仓库，负责存储扫描结果并提供全量替换、增量更新、快照等功能。
///        数据与 UI 解耦，是扫描架构中的唯一数据源。
class ScanResultRepository {
public:
    // ----- 数据修改 -----

    /// 用全新的结果集替换当前所有数据。
    /// 会根据旧结果自动继承 lastValue / firstValue，保持 CE 风格。
    void replaceAllResults(const std::vector<ScanResult>& newResults);

    /// 行级增量更新：仅刷新指定行的 value 和 changed 标志。
    /// @param generation 调用方持有的版本号，不匹配则放弃更新。
    void applyIncrementalUpdates(int generation,
        const std::vector<int>& rows,
        const std::vector<uint64_t>& newValues,
        const std::vector<uint8_t>& changedFlags);

    // ----- 数据查询 -----

    /// 返回当前结果总数
    [[nodiscard]] size_t resultCount() const;

    /// 根据索引获取结果指针，越界返回 nullptr
    [[nodiscard]] const ScanResult* resultAt(size_t index) const;

    /// 根据索引获取地址，越界返回 0
    [[nodiscard]] uint64_t addressAtIndex(size_t index) const;

    // ----- 快照与版本 -----

    /// 创建当前数据的只读共享快照，用于线程安全的高性能遍历
    [[nodiscard]] std::shared_ptr<const std::vector<ScanResult>> createSnapshot() const;

    /// 返回当前数据版本号，每次 replace 后递增
    [[nodiscard]] int currentGeneration() const;

private:
    mutable std::mutex m_mutex;
    std::vector<ScanResult> m_data;
    std::atomic<int> m_generation{ 0 };

    mutable std::shared_ptr<const std::vector<ScanResult>> m_snapshotCache;
    mutable int m_snapshotGeneration{ 0 };
};