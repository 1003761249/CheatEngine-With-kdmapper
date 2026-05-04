#include "scan_engine.h"
#include "process_manager.h"
#include "thread_pool.h"
#include "SimdKernels.h" // 引入你刚才添加的类
#include <cstring>
#include <algorithm>
#include <future>
#include <thread>


#define AdaptiveCachePool_SIZE 10240

// ============================================================================
// 内部工具：类型安全的转换 (Type Punning)
// 防止违反 Strict Aliasing 规则，现代编译器会将其优化为零开销指令
// ============================================================================
namespace {
	// 将 64 位原始数据解包为目标泛型 T
	template <typename T>
	inline T unpackValue(uint64_t raw) {
		T val = 0;
		std::memcpy(&val, &raw, std::min(sizeof(T), sizeof(uint64_t)));
		return val;
	}

	// 将泛型 T 封包为 64 位原始数据存储
	template <typename T>
	inline uint64_t packValue(T val) {
		uint64_t raw = 0;
		std::memcpy(&raw, &val, sizeof(T));
		return raw;
	}
}

// ============================================================================
// 核心对外接口
// ============================================================================

ScanEngine::ResultPack ScanEngine::execute(const ScanRequest& request) {
	// 1. 重置引擎状态
	m_cancel.store(false, std::memory_order_release);
	m_regionsCompleted.store(0, std::memory_order_relaxed);
	m_totalRegions = 0;

	// 2. 初始化自适应缓存容器：设定内存阈值为 200,000 个元素
	// 超过此数量后，缓存类会自动向磁盘转储，引擎对此无感
	auto cache = std::make_shared<AdaptiveCachePool<ScanResult>>(AdaptiveCachePool_SIZE);

	// 3. 顶层路由分发
	if (request.mode == ScanMode::First) {
		switch (request.dataType) {
		case ScanDataType::Int8:    dispatchFirstScanForType<int8_t>(request, *cache); break;
		case ScanDataType::Int16:   dispatchFirstScanForType<int16_t>(request, *cache); break;
		case ScanDataType::Int32:   dispatchFirstScanForType<int32_t>(request, *cache); break;
		case ScanDataType::Int64:   dispatchFirstScanForType<int64_t>(request, *cache); break;
		case ScanDataType::Float32: dispatchFirstScanForType<float>(request, *cache); break;
		case ScanDataType::Float64: dispatchFirstScanForType<double>(request, *cache); break;
			// 字符串和 AOB 模式预留给未来扩展
		case ScanDataType::AsciiString:
		case ScanDataType::Utf16String:
		case ScanDataType::ByteArray:
			break;
		}
	}
	else if (request.mode == ScanMode::Next && request.prevResults) {

		if (!request.prevResults || request.prevResults->empty()) {
			// 如果是 Unknown Initial 后的第一次 Next Scan，previousResults 确实为空
			// 但此时 dispatchNextScanForType 内部会去处理快照对比逻辑
		}

		// 获取结果引用
		const std::vector<ScanResult>& prevResults = request.prevResults ?
			*request.prevResults :
			std::vector<ScanResult>();

		switch (request.dataType) {
		case ScanDataType::Int8:    dispatchNextScanForType<int8_t>(request, prevResults, *cache); break;
		case ScanDataType::Int16:   dispatchNextScanForType<int16_t>(request, prevResults, *cache); break;
		case ScanDataType::Int32:   dispatchNextScanForType<int32_t>(request, prevResults, *cache); break;
		case ScanDataType::Int64:   dispatchNextScanForType<int64_t>(request, prevResults, *cache); break;
		case ScanDataType::Float32: dispatchNextScanForType<float>(request, prevResults, *cache); break;
		case ScanDataType::Float64: dispatchNextScanForType<double>(request, prevResults, *cache); break;
		default: break;
		}
	}

	// 4. 返回打包结果
	return { cache, request.dataType };
}

// ============================================================================
// 阶段 1：初次扫描分发器 (First Scan Dispatcher)
// 将动态的枚举请求转化为静态的 C++ 模板实例，并绑定 Lambda 谓词算子
// ============================================================================
template <typename T>
void ScanEngine::dispatchFirstScanForType(const ScanRequest& req, AdaptiveCachePool<ScanResult>& outCache) {
	auto valParams = std::get<ValueParams>(req.params);
	T v1 = unpackValue<T>(valParams.value1);
	T v2 = unpackValue<T>(valParams.value2);

	switch (req.firstType) {
	case ScanType::ExactValue:
		executeFirstScanCore<T>(req, [v1](T val) { return val == v1; }, outCache); break;
	case ScanType::GreaterThan:
		executeFirstScanCore<T>(req, [v1](T val) { return val > v1; }, outCache); break;
	case ScanType::LessThan:
		executeFirstScanCore<T>(req, [v1](T val) { return val < v1; }, outCache); break;
	case ScanType::Between:
		// 确保 v1 是较小值，v2 是较大值
		executeFirstScanCore<T>(req, [minV = std::min(v1, v2), maxV = std::max(v1, v2)](T val) {
			return val >= minV && val <= maxV;
			}, outCache); break;
	case ScanType::UnknownInitial:
		executeFirstScanCore<T>(req, [](T) { return true; }, outCache); break;
	default: break;
	}
}


template <typename T>
std::function<bool(T, T)> ScanEngine::getNextScanPredicate(const ScanRequest& req) {
	T targetValue = 0;
	T secondValue = 0;

	// 解析参数
	if (std::holds_alternative<ValueParams>(req.params)) {
		const auto& p = std::get<ValueParams>(req.params);
		targetValue = unpackValue<T>(p.value1);
		secondValue = unpackValue<T>(p.value2);
	}

	// 必须明确指定返回类型为 std::function<bool(T, T)>
	switch (req.nextType) {
	case NextScanType::Equal:
		return [targetValue](T cur, T old) -> bool { return cur == targetValue; };

	case NextScanType::NotEqual:
		return [targetValue](T cur, T old) -> bool { return cur != targetValue; };

	case NextScanType::Increased:
		return [](T cur, T old) -> bool { return cur > old; };

	case NextScanType::Decreased:
		return [](T cur, T old) -> bool { return cur < old; };

	case NextScanType::Changed:
		return [](T cur, T old) -> bool { return cur != old; };

	case NextScanType::Unchanged:
		return [](T cur, T old) -> bool { return cur == old; };

	case NextScanType::Between:
		return [targetValue, secondValue](T cur, T old) -> bool {
			T minV = std::min(targetValue, secondValue);
			T maxV = std::max(targetValue, secondValue);
			return cur >= minV && cur <= maxV;
			};

	default:
		return [](T cur, T old) -> bool { return false; };
	}
}


// scan_engine.cpp

template <typename T>
void ScanEngine::dispatchNextScanForType(const ScanRequest& req,
	const std::vector<ScanResult>& prevResults,
	AdaptiveCachePool<ScanResult>& outCache)
{
	auto pred = getNextScanPredicate<T>(req);

	bool isFirstNextAfterUnknown = (req.firstType == ScanType::UnknownInitial && prevResults.empty());
	// 如果是“未知初始值”扫描后的“再次扫描”
	if (isFirstNextAfterUnknown) {
		// 情况 A：首扫是未知值，当前结果集为空 -> 从快照文件中读取并对比
		executeNextScanAfterUnknown<T>(req, pred, outCache);
	}
	else if (!prevResults.empty()) {
		// 情况 B：正常基于上一次搜索结果的再次扫描
		executeNextScanCore<T>(req, prevResults, pred, outCache);
	}
	else {
		return;
		// 正常基于上一次结果的再次扫描
		//executeNextScanCore<T>(req, prevResults, pred, outCache);
	}
}


void ScanEngine::createMemorySnapshot(const std::vector<MemoryRegion>& regions) {
	auto mem = ProcessManager::instance().memory();

	// 1. 初始化文件与清空旧索引
	std::ofstream outFile(m_snapshotPath, std::ios::binary);
	m_snapshotIndex.clear();

	size_t currentFileOffset = 0;

	for (const auto& region : regions) {
		if (isCancelled()) break;

		// 2. 记录当前内存块在文件中的起始偏移量
		m_snapshotIndex[region.base] = currentFileOffset;

		std::vector<uint8_t> buffer(512 * 1024);
		for (uint64_t curr = region.base; curr < region.base + region.size; curr += buffer.size()) {
			size_t toRead = std::min(buffer.size(), (size_t)(region.base + region.size - curr));

			if (mem->read(curr, buffer.data(), toRead)) {
				outFile.write(reinterpret_cast<char*>(buffer.data()), toRead);
			}
			else {
				// 【关键修复】如果读取失败，必须用零填充文件占位！
				// 否则后续通过偏移量 (address - regionBase) 寻找数据时会产生错位。
				std::vector<uint8_t> zeros(toRead, 0);
				outFile.write(reinterpret_cast<char*>(zeros.data()), toRead);
			}
			currentFileOffset += toRead;
		}
	}
}


template <typename T, typename Predicate>
void ScanEngine::executeFirstScanCore(const ScanRequest& req, Predicate pred, AdaptiveCachePool<ScanResult>& outCache) {
	auto mem = ProcessManager::instance().memory();
	auto regions = ProcessManager::instance().regionEnumerator()->enumerate();

	m_totalRegions = static_cast<int>(regions.size());

	// 定义扫描步进：SIMD 通常要求按类型大小对齐才有意义
	size_t step = (req.alignment > 0) ? req.alignment : 1;
	size_t typeSize = sizeof(T);

	// 设置本地读取缓冲区：512KB 是平衡系统调用开销与 L3 缓存命中的黄金值
	const size_t CHUNK_SIZE = 512 * 1024;
	std::vector<uint8_t> buffer(CHUNK_SIZE);

	for (const auto& region : regions) {
		if (isCancelled()) break;

		if (req.firstType == ScanType::UnknownInitial) {
			this->createMemorySnapshot(regions);
			return; // 直接返回，不生成结果集
		}

		for (uint64_t curr = region.base; curr < region.base + region.size; curr += CHUNK_SIZE) {
			if (isCancelled()) break;

			size_t toRead = std::min(CHUNK_SIZE, (size_t)(region.base + region.size - curr));

			// 1. 批量读取内存，极大地减少系统调用
			if (!mem->read(curr, buffer.data(), toRead)) {
				continue;
			}

			std::vector<ScanResult> localBatch;
			localBatch.reserve(4096);
			std::vector<uint64_t> hits; // 存放 SIMD 命中的地址
			bool processedBySimd = false;

			// 2. SIMD 分发逻辑：仅在 ExactValue 且对齐时启用
			if (req.firstType == ScanType::ExactValue && step == typeSize) {
				const auto& params = std::get<ValueParams>(req.params);

				if constexpr (std::is_same_v<T, int8_t>) {
					SimdScanner::scanInt8Exact(buffer.data(), toRead, static_cast<int8_t>(params.value1), curr, hits);
					processedBySimd = true;
				}
				else if constexpr (std::is_same_v<T, int16_t>) {
					SimdScanner::scanInt16Exact(buffer.data(), toRead, static_cast<int16_t>(params.value1), curr, hits);
					processedBySimd = true;
				}
				else if constexpr (std::is_same_v<T, int32_t>) {
					SimdScanner::scanInt32Exact(buffer.data(), toRead, static_cast<int32_t>(params.value1), curr, hits);
					processedBySimd = true;
				}
				else if constexpr (std::is_same_v<T, int64_t>) {
					SimdScanner::scanInt64Exact(buffer.data(), toRead, static_cast<int64_t>(params.value1), curr, hits);
					processedBySimd = true;
				}
				else if constexpr (std::is_same_v<T, float>) {
					float target;
					uint64_t raw = params.value1;
					std::memcpy(&target, &raw, sizeof(float));
					SimdScanner::scanFloatExact(buffer.data(), toRead, target, curr, hits);
					processedBySimd = true;
				}
				else if constexpr (std::is_same_v<T, double>) {
					double target;
					uint64_t raw = params.value1;
					std::memcpy(&target, &raw, sizeof(double));
					SimdScanner::scanDoubleExact(buffer.data(), toRead, target, curr, hits);
					processedBySimd = true;
				}
			}
			// 范围扫描 (Between) SIMD 扩展 (以 Int32 为例)
			else if (req.firstType == ScanType::Between && step == typeSize) {
				if constexpr (std::is_same_v<T, int32_t>) {
					const auto& params = std::get<ValueParams>(req.params);
					SimdScanner::scanInt32Range(buffer.data(), toRead, (int32_t)params.value1, (int32_t)params.value2, curr, hits);
					processedBySimd = true;
				}
			}

			// 3. 处理 SIMD 命中的结果
			if (processedBySimd) {
				for (uint64_t addr : hits) {
					ScanResult res;
					res.address = addr;
					// 对于 ExactValue，值是已知的，避免重复的 unpack 逻辑
					res.value = (req.firstType == ScanType::ExactValue) ?
						std::get<ValueParams>(req.params).value1 : 0;

					// 如果是 Between，则需要从 buffer 提取真实值（或者此处不提取以换取速度）
					if (req.firstType != ScanType::ExactValue) {
						T val;
						std::memcpy(&val, buffer.data() + (addr - curr), sizeof(T));
						res.value = packValue<T>(val);
					}

					res.firstValue = res.value;
					res.lastValue = res.value;
					res.changed = false;
					localBatch.push_back(res);

					if (localBatch.size() >= 4096) {
						outCache.push_back_batch(localBatch);
						localBatch.clear();
					}
				}
			}
			else {
				// 4. 标量回退：处理 SIMD 不支持的类型或特殊步长（如字节对齐扫描 Int32）
				for (size_t offset = 0; offset <= toRead - typeSize; offset += step) {
					T val;
					std::memcpy(&val, buffer.data() + offset, typeSize);

					if (pred(val)) {
						ScanResult res;
						res.address = curr + offset;
						res.value = packValue<T>(val);
						res.firstValue = res.value;
						res.lastValue = res.value;
						res.changed = false;
						localBatch.push_back(res);

						if (localBatch.size() >= 4096) {
							outCache.push_back_batch(localBatch);
							localBatch.clear();
						}
					}
				}
			}

			if (!localBatch.empty()) {
				outCache.push_back_batch(localBatch);
			}
		}
		m_regionsCompleted.fetch_add(1, std::memory_order_relaxed);
	}
}

template <typename T, typename Predicate>
void ScanEngine::executeNextScanCore(const ScanRequest& req, const std::vector<ScanResult>& prevResults, Predicate pred, AdaptiveCachePool<ScanResult>& outCache) {
	auto mem = ProcessManager::instance().memory();
	if (!mem || !req.prevResults) return;

	const std::vector<ScanResult>& prevSet = *(req.prevResults);
	size_t total = prevSet.size();
	if (total == 0) return;

	// 按线程数均匀切分快照向量
	size_t threads = std::max<size_t>(1, std::thread::hardware_concurrency());
	size_t chunkSize = (total + threads - 1) / threads;
	std::vector<std::future<void>> tasks;

	for (size_t start = 0; start < total; start += chunkSize) {
		tasks.push_back(GlobalThreadPool::instance().enqueue([&, start, chunkSize]() {
			if (isCancelled()) return;

			std::vector<ScanResult> localBatch;
			localBatch.reserve(4096);
			size_t end = std::min(start + chunkSize, total);

			// 遍历切分好的快照切片
			for (size_t i = start; i < end; ++i) {
				if (isCancelled()) break;

				// 直接索引 std::vector，性能极高
				const ScanResult& oldRes = prevSet[i];

				T oldVal = unpackValue<T>(oldRes.lastValue);
				T curVal;

				// 读取最新内存值并验证
				if (mem->read(oldRes.address, &curVal, sizeof(T))) {
					if (pred(curVal, oldVal)) {
						ScanResult newRes;
						newRes.address = oldRes.address;
						newRes.value = packValue<T>(curVal);
						newRes.lastValue = oldRes.value;        // 继承上一次的值
						newRes.firstValue = oldRes.firstValue;  // 继承首扫的值
						newRes.changed = (newRes.value != newRes.lastValue);

						localBatch.push_back(newRes);

						if (localBatch.size() >= 4096) {
							outCache.push_back_batch(localBatch);
							localBatch.clear();
						}
					}
				}
			}
			if (!localBatch.empty()) {
				outCache.push_back_batch(localBatch);
			}
			}));
	}

	// 等待所有切片验证完毕
	for (auto& t : tasks) {
		if (t.valid()) t.wait();
	}
}


template <typename T, typename Predicate>
void ScanEngine::executeNextScanAfterUnknown(const ScanRequest& req, Predicate pred, AdaptiveCachePool<ScanResult>& outCache) {
	auto mem = ProcessManager::instance().memory();
	auto regions = ProcessManager::instance().regionEnumerator()->enumerate();

	m_totalRegions = static_cast<int>(regions.size());
	const size_t CHUNK_SIZE = 512 * 1024;
	std::vector<uint8_t> curBuf(CHUNK_SIZE);
	std::vector<uint8_t> oldBuf(CHUNK_SIZE);

	std::ifstream snapFile(m_snapshotPath, std::ios::binary);
	if (!snapFile) return;

	for (const auto& region : regions) {
		if (isCancelled()) break;

		for (uint64_t curr = region.base; curr < region.base + region.size; curr += CHUNK_SIZE) {
			if (isCancelled()) break;

			size_t toRead = std::min(CHUNK_SIZE, (size_t)(region.base + region.size - curr));
			if (!mem->read(curr, curBuf.data(), toRead)) continue;

			// 这里你需要从你保存快照的地方读取数据。
			// 假设你存成了文件或内存块。如果没有实现快照读取，这里会报错。
			if (!this->readSnapshotDataOptimized(snapFile,curr, oldBuf.data(), toRead)) continue;

			std::vector<ScanResult> localBatch;

			// --- SIMD 加速部分 ---
			bool handledBySimd = false;
			if constexpr (std::is_same_v<T, int32_t>) {
				if (req.alignment == 4) {
					std::vector<uint64_t> hits;
					if (req.nextType == NextScanType::Decreased) {
						SimdScanner::scanInt32Decreased(curBuf.data(), oldBuf.data(), toRead, curr, hits);
						handledBySimd = true;
					}
					else if (req.nextType == NextScanType::Increased) {
						SimdScanner::scanInt32Increased(curBuf.data(), oldBuf.data(), toRead, curr, hits);
						handledBySimd = true;
					}

					if (handledBySimd) {
						for (uint64_t addr : hits) {
							int32_t cV, oV;
							std::memcpy(&cV, curBuf.data() + (addr - curr), 4);
							std::memcpy(&oV, oldBuf.data() + (addr - curr), 4);

							ScanResult res{ addr, packValue<int32_t>(cV), packValue<int32_t>(oV), packValue<int32_t>(oV), (cV != oV) };
							localBatch.push_back(res);
						}
					}
				}
			}

			// --- 标量回退部分 ---
			if (!handledBySimd) {
				size_t step = (req.alignment > 0) ? req.alignment : sizeof(T);
				for (size_t offset = 0; offset <= toRead - sizeof(T); offset += step) {
					T cV, oV;
					std::memcpy(&cV, curBuf.data() + offset, sizeof(T));
					std::memcpy(&oV, oldBuf.data() + offset, sizeof(T));

					if (pred(cV, oV)) {
						ScanResult res;
						res.address = curr + offset;
						res.value = packValue<T>(cV);
						res.lastValue = packValue<T>(oV);
						res.firstValue = res.lastValue;
						res.changed = (res.value != res.lastValue);
						localBatch.push_back(res);

						if (localBatch.size() >= 4096) {
							outCache.push_back_batch(localBatch);
							localBatch.clear();
						}
					}
				}
			}
			if (!localBatch.empty()) outCache.push_back_batch(localBatch);
		}
		m_regionsCompleted.fetch_add(1);
	}
}


bool ScanEngine::readSnapshotDataOptimized(std::ifstream& inFile, uint64_t address, uint8_t* buffer, size_t size) {
	auto it = m_snapshotIndex.upper_bound(address);
	if (it == m_snapshotIndex.begin()) return false;
	--it;

	size_t readOffset = it->second + (address - it->first);
	inFile.seekg(readOffset);
	inFile.read(reinterpret_cast<char*>(buffer), size);
	return inFile.gcount() == size;
}

bool ScanEngine::readSnapshotData(uint64_t address, uint8_t* buffer, size_t size) {
	if (m_snapshotIndex.empty()) return false;

	// 1. 使用 upper_bound 找到第一个基址大于 address 的 Region，退一步就是包含 address 的 Region
	auto it = m_snapshotIndex.upper_bound(address);
	if (it == m_snapshotIndex.begin()) return false;
	--it;

	uint64_t regionBase = it->first;
	size_t fileOffset = it->second;

	// 2. 计算实际需要读取的文件偏移量
	size_t readOffset = fileOffset + (address - regionBase);

	// 3. 打开快照文件读取
	// 注意：为了极致性能，这里最好不要每次都打开文件，
	// 但作为初步修复，这个写法最稳健，后续你可以考虑将 ifstream 提取到类级别
	std::ifstream inFile(m_snapshotPath, std::ios::binary);
	if (!inFile) return false;

	inFile.seekg(readOffset);
	inFile.read(reinterpret_cast<char*>(buffer), size);

	// 如果成功读取的字节数符合预期，则返回 true
	return inFile.gcount() == size;
}