#pragma once
#include <vector>
#include <string>
#include <mutex>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <stdexcept>
#include <map>
#include <memory>
#include <atomic>
#include <thread>

#define THEAD_LOCAL_SIZE  50LL * 1024  //5MB


/// @brief 泛型自适应缓存容器（子缓存单元）
template <typename T>
class AdaptiveCache {
    static_assert(std::is_trivially_copyable_v<T>, "AdaptiveCache requires trivially copyable types.");

public:
    // 【修改】构造函数增加唯一标识，防止多线程创建文件冲突
    explicit AdaptiveCache(size_t memThreshold = 100'000, std::string id = "default")
        : m_threshold(memThreshold), m_count(0), m_onDisk(false), m_uniqueId(id) {
        m_buffer.reserve(std::min(memThreshold, (size_t)10240));
    }

    ~AdaptiveCache() { clear(); }

    // 禁用拷贝，支持移动
    AdaptiveCache(AdaptiveCache&& other) noexcept { moveFrom(std::move(other)); }
    AdaptiveCache& operator=(AdaptiveCache&& other) noexcept {
        if (this != &other) { clear(); moveFrom(std::move(other)); }
        return *this;
    }

    AdaptiveCache(const AdaptiveCache&) = delete;
    AdaptiveCache& operator=(const AdaptiveCache&) = delete;

    void push_back(const T& item) {
        std::lock_guard<std::mutex> lock(m_mtx);
        if (!m_onDisk) {
            m_buffer.push_back(item);
            if (m_buffer.size() >= m_threshold) flushToDisk();
        }
        else {
            ensureFileOpen();
            m_diskOut.write(reinterpret_cast<const char*>(&item), sizeof(T));
        }
        ++m_count;
    }

    //void push_back_batch(const std::vector<T>& items) {
    //    if (items.empty()) return;
    //    std::lock_guard<std::mutex> lock(m_mtx);
    //    if (!m_onDisk && (m_buffer.size() + items.size() < m_threshold)) {
    //        m_buffer.insert(m_buffer.end(), items.begin(), items.end());
    //    }
    //    else {
    //        if (!m_onDisk) flushToDisk();
    //        ensureFileOpen();
    //        m_diskOut.write(reinterpret_cast<const char*>(items.data()), items.size() * sizeof(T));
    //    }
    //    m_count += items.size();
    //}

    void push_back_batch(const std::vector<T>& items) {
        if (items.empty()) return;
        std::lock_guard<std::mutex> lock(m_mtx);

        // 只有当前已经在磁盘，或者 内存+新数据 超过阈值时，才写磁盘
        if (m_onDisk || (m_buffer.size() + items.size() >= m_threshold)) {
            if (!m_onDisk) flushToDisk(); // 第一次超过阈值，把内存里的全倒进去

            ensureFileOpen();
            m_diskOut.write(reinterpret_cast<const char*>(items.data()), items.size() * sizeof(T));
            // 注意：不要在这里调用 flush()，让操作系统自己调度合并写入，提高性能
        }
        else {
            // 还没到阈值，直接进内存
            m_buffer.insert(m_buffer.end(), items.begin(), items.end());
        }
        m_count += items.size();
    }



    size_t size() const {
        std::lock_guard<std::mutex> lock(m_mtx);
        return m_count;
    }

    // 从当前子缓存读取切片
    std::vector<T> readChunk(size_t start, size_t count) const {
        std::lock_guard<std::mutex> lock(m_mtx);
        std::vector<T> chunk;
        if (start >= m_count) return chunk;

        size_t end = std::min(start + count, m_count);
        size_t actualToRead = end - start;
        chunk.reserve(actualToRead);

        if (!m_onDisk) {
            chunk.assign(m_buffer.begin() + start, m_buffer.begin() + end);
        }
        else {
            // 注意：读取前确保写入流已关闭或 flush，这里使用独立的 ifstream
            std::ifstream file(m_diskPath, std::ios::binary);
            if (file) {
                file.seekg(start * sizeof(T));
                T tmp;
                for (size_t i = 0; i < actualToRead && file.read(reinterpret_cast<char*>(&tmp), sizeof(T)); ++i) {
                    chunk.push_back(tmp);
                }
            }
        }
        return chunk;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(m_mtx);
        m_buffer.clear();
        m_buffer.shrink_to_fit();
        if (m_diskOut.is_open()) m_diskOut.close();
        if (!m_diskPath.empty()) {
            std::error_code ec;
            std::filesystem::remove(m_diskPath, ec);
            m_diskPath.clear();
        }
        m_count = 0;
        m_onDisk = false;
    }

private:
    void moveFrom(AdaptiveCache&& other) {
        std::lock_guard<std::mutex> lock(other.m_mtx);
        m_buffer = std::move(other.m_buffer);
        m_diskPath = std::move(other.m_diskPath);
        m_diskOut = std::move(other.m_diskOut);
        m_count = other.m_count;
        m_threshold = other.m_threshold;
        m_onDisk = other.m_onDisk;
        m_uniqueId = std::move(other.m_uniqueId);
        other.m_count = 0;
        other.m_onDisk = false;
    }

    void ensureFileOpen() {
        if (!m_diskOut.is_open()) {
            if (m_diskPath.empty()) {
                auto dir = std::filesystem::temp_directory_path();
                auto ts = std::chrono::high_resolution_clock::now().time_since_epoch().count();
                // 【修改】文件名包含唯一标识，避免冲突
                m_diskPath = (dir / ("ACache_" + m_uniqueId + "_" + std::to_string(ts) + ".tmp")).string();
            }
            m_diskOut.open(m_diskPath, std::ios::binary | std::ios::app);
        }
    }

    void flushToDisk() {
        if (m_buffer.empty()) return;
        ensureFileOpen();
        m_diskOut.write(reinterpret_cast<const char*>(m_buffer.data()), m_buffer.size() * sizeof(T));
        m_diskOut.flush(); // 确保写入磁盘
        m_buffer.clear();
        m_buffer.shrink_to_fit();
        m_onDisk = true;
    }

    mutable std::mutex m_mtx;
    std::vector<T> m_buffer;
    std::string m_diskPath;
    std::ofstream m_diskOut;
    size_t m_count;
    size_t m_threshold;
    bool m_onDisk;
    std::string m_uniqueId; // 【新增】实例唯一 ID
};

// =================================================================
// 【新增】AdaptiveCachePool：管理多个扫描线程的子缓存
// =================================================================
template <typename T>
class AdaptiveCachePool {
public:
    explicit AdaptiveCachePool(size_t perThreadThreshold = THEAD_LOCAL_SIZE)
        : m_perThreadThreshold(perThreadThreshold) {
    }

    /// @brief 获取当前线程专属的子缓存并添加数据（完全无锁竞争）
    void push_back(const T& item) {
        getThreadLocalCache().push_back(item);
    }

    void push_back_batch(const std::vector<T>& items) {
        if (items.empty()) return;
        // getThreadLocalCache 会自动返回当前线程关联的 AdaptiveCache 实例
        getThreadLocalCache().push_back_batch(items);
    }

    /// @brief 获取总元素数量
    size_t total_size() const {
        std::lock_guard<std::mutex> lock(m_poolMtx);
        size_t total = 0;
        for (const auto& [id, cache] : m_subCaches) {
            total += cache->size();
        }
        return total;
    }

    /// @brief 统一分页读取：聚合所有子缓存的数据
    std::vector<T> readChunk(size_t start, size_t count) const {
        std::lock_guard<std::mutex> lock(m_poolMtx);
        std::vector<T> result;
        size_t currentOffset = 0;
        size_t remainingToRead = count;

        for (const auto& [id, cache] : m_subCaches) {
            size_t cacheSize = cache->size();
            // 检查当前 chunk 是否落在这个子缓存的范围内
            if (start < currentOffset + cacheSize) {
                size_t internalStart = (start > currentOffset) ? (start - currentOffset) : 0;
                size_t internalCount = std::min(remainingToRead, cacheSize - internalStart);

                auto subChunk = cache->readChunk(internalStart, internalCount);
                result.insert(result.end(), subChunk.begin(), subChunk.end());

                remainingToRead -= subChunk.size();
                start += subChunk.size(); // 移动起点
                if (remainingToRead == 0) break;
            }
            currentOffset += cacheSize;
        }
        return result;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(m_poolMtx);
        for (auto& [id, cache] : m_subCaches) {
            cache->clear();
        }
        m_subCaches.clear();
    }

private:
    /// @brief 核心方法：为每个线程按需创建或返回已有的子缓存
    AdaptiveCache<T>& getThreadLocalCache() {
        thread_local std::string tid = [] {
            std::stringstream ss;
            ss << std::this_thread::get_id();
            return ss.str();
            }();

        // 只有在第一次创建该线程的子缓存时才加锁
        std::lock_guard<std::mutex> lock(m_poolMtx);
        auto it = m_subCaches.find(tid);
        if (it == m_subCaches.end()) {
            auto newCache = std::make_unique<AdaptiveCache<T>>(m_perThreadThreshold, tid);
            auto& ref = *newCache;
            m_subCaches[tid] = std::move(newCache);
            return ref;
        }
        return *(it->second);
    }

    mutable std::mutex m_poolMtx;
    size_t m_perThreadThreshold;
    std::map<std::string, std::unique_ptr<AdaptiveCache<T>>> m_subCaches;
};