#include "scan_result_repository.h"
#include <unordered_map>
#include <algorithm>

void ScanResultRepository::replaceAllResults(const std::vector<ScanResult>& newResults)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // 构建旧地址 → 旧结果的映射，用于继承 lastValue / firstValue
    std::unordered_map<uint64_t, ScanResult> oldMap;
    for (const auto& old : m_data) {
        oldMap[old.address] = old;
    }

    m_data = newResults;
    for (auto& item : m_data) {
        auto it = oldMap.find(item.address);
        if (it != oldMap.end()) {
            item.lastValue = it->second.value;
            item.firstValue = it->second.firstValue;
            item.changed = false;
        }
        else {
            item.lastValue = item.value;
            item.firstValue = item.value;
            item.changed = false;
        }
    }

    // 快照失效
    m_snapshotCache.reset();
    m_snapshotGeneration = 0;

    m_generation.fetch_add(1, std::memory_order_release);
}

void ScanResultRepository::applyIncrementalUpdates(int generation,
    const std::vector<int>& rows,
    const std::vector<uint64_t>& newValues,
    const std::vector<uint8_t>& changedFlags)
{
    // 版本检查（快速路径）
    if (generation != m_generation.load(std::memory_order_acquire))
        return;
    if (rows.empty())
        return;

    std::lock_guard<std::mutex> lock(m_mutex);
    // 获取锁后再次验证，防止 replaceAllResults 插队
    if (generation != m_generation.load(std::memory_order_acquire))
        return;

    for (size_t i = 0; i < rows.size(); ++i) {
        int row = rows[i];
        if (row < 0 || row >= static_cast<int>(m_data.size()))
            continue;

        auto& item = m_data[row];
        item.value = newValues[i];
        item.changed = changedFlags[i];
    }

    m_snapshotCache.reset();
    m_snapshotGeneration = 0;
}

size_t ScanResultRepository::resultCount() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_data.size();
}

const ScanResult* ScanResultRepository::resultAt(size_t index) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (index >= m_data.size())
        return nullptr;
    return &m_data[index];
}

uint64_t ScanResultRepository::addressAtIndex(size_t index) const
{
    auto* res = resultAt(index);
    return res ? res->address : 0;
}

std::shared_ptr<const std::vector<ScanResult>> ScanResultRepository::createSnapshot() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    int gen = m_generation.load(std::memory_order_acquire);

    if (m_snapshotCache && m_snapshotGeneration == gen)
        return m_snapshotCache;

    m_snapshotCache = std::make_shared<const std::vector<ScanResult>>(m_data);
    m_snapshotGeneration = gen;
    return m_snapshotCache;
}

int ScanResultRepository::currentGeneration() const
{
    return m_generation.load();
}