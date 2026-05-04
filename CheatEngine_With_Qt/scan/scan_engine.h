#pragma once

#include "scan_request.h"
#include "scan_result.h"
#include "scan_types.h"
#include "adaptive_cache.h"
#include "memory_region.h"
#include <atomic>
#include <memory>
#include <map>
#include <functional> 

class ScanEngine {
public:
    // 包含自适应缓存的返回包
    struct ResultPack {
        std::shared_ptr<AdaptiveCachePool<ScanResult>> results;
        ScanDataType dataType;
    };

    ScanEngine() = default;

    ResultPack execute(const ScanRequest& request);

    void cancel() { m_cancel.store(true, std::memory_order_release); }
    bool isCancelled() const { return m_cancel.load(std::memory_order_acquire); }
    int regionsCompleted() const { return m_regionsCompleted.load(std::memory_order_relaxed); }
    int totalRegions() const { return m_totalRegions; }
    bool hasSnapshot() const { return !m_snapshotIndex.empty(); }


private:
    void createMemorySnapshot(const std::vector<MemoryRegion>& regions);
    bool readSnapshotDataOptimized(std::ifstream& inFile, uint64_t address, uint8_t* buffer, size_t size);
    //未知初始值扫描优化
    bool readSnapshotData(uint64_t address, uint8_t* buffer, size_t size);

    template <typename T>
    std::function<bool(T, T)> getNextScanPredicate(const ScanRequest& req);

    // 引擎分发与核心逻辑 (使用泛型 T 作为数据类型)
    template <typename T>
    void dispatchFirstScanForType(const ScanRequest& req, AdaptiveCachePool<ScanResult>& outCache);

    template <typename T>
    void dispatchNextScanForType(const ScanRequest& req,
        const std::vector<ScanResult>& prevResults,
        AdaptiveCachePool<ScanResult>& outCache);

    template <typename T, typename Predicate>
    void executeFirstScanCore(const ScanRequest& req, Predicate pred, AdaptiveCachePool<ScanResult>& outCache);

    template <typename T, typename Predicate>
    void executeNextScanCore(const ScanRequest& req, const std::vector<ScanResult>& prevResults, Predicate pred, AdaptiveCachePool<ScanResult>& outCache);
    
    template <typename T, typename Predicate>
    void executeNextScanAfterUnknown(const ScanRequest& req, Predicate pred, AdaptiveCachePool<ScanResult>& outCache);
   
    std::atomic<bool> m_cancel{ false };
    std::atomic<int>  m_regionsCompleted{ 0 };
    int               m_totalRegions = 0;

    std::string m_snapshotPath = "./temp_snapshot.bin";
    std::map<uint64_t, size_t>  m_snapshotIndex;
};