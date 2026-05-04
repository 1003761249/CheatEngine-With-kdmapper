#pragma once

#include <immintrin.h>
#include <cstdint>
#include <vector>
#include <cmath>
#include <type_traits>

class SimdScanner {
public:
    // ============================================================
    // 1. 整数类型通用精确扫描 (Int8, Int16, Int32)
    // ============================================================

    // Int8 (Byte) 加速：一次比较 32 个字节
    static void scanInt8Exact(const uint8_t* buffer, size_t size, int8_t target, uint64_t baseAddr, std::vector<uint64_t>& results) {
        __m256i vTarget = _mm256_set1_epi8(target);
        for (size_t i = 0; i + 31 < size; i += 32) {
            __m256i vData = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(buffer + i));
            __m256i vMask = _mm256_cmpeq_epi8(vData, vTarget);
            int mask = _mm256_movemask_epi8(vMask);
            if (mask != 0) {
                for (int j = 0; j < 32; ++j) {
                    if (mask & (1LL << j)) results.push_back(baseAddr + i + j);
                }
            }
        }
        // 尾部标量处理留给外部或在此补充
    }

    // Int16 (Short) 加速：一次比较 16 个 Short
    static void scanInt16Exact(const uint8_t* buffer, size_t size, int16_t target, uint64_t baseAddr, std::vector<uint64_t>& results) {
        __m256i vTarget = _mm256_set1_epi16(target);
        for (size_t i = 0; i + 31 < size; i += 32) {
            __m256i vData = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(buffer + i));
            __m256i vMask = _mm256_cmpeq_epi16(vData, vTarget);
            // epi16 没有直接的 movemask，使用 epi8 的替代
            int mask = _mm256_movemask_epi8(vMask);
            if (mask != 0) {
                for (int j = 0; j < 16; ++j) {
                    if (mask & (0x3 << (j * 2))) results.push_back(baseAddr + i + (j * 2));
                }
            }
        }
    }

    // Int32 (Dword) 加速：一次比较 8 个 Int32
    static void scanInt32Exact(const uint8_t* buffer, size_t size, int32_t target, uint64_t baseAddr, std::vector<uint64_t>& results) {
        __m256i vTarget = _mm256_set1_epi32(target);
        for (size_t i = 0; i + 31 < size; i += 32) {
            __m256i vData = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(buffer + i));
            __m256i vMask = _mm256_cmpeq_epi32(vData, vTarget);
            int mask = _mm256_movemask_ps(_mm256_castsi256_ps(vMask));
            if (mask != 0) {
                for (int j = 0; j < 8; ++j) {
                    if (mask & (1 << j)) results.push_back(baseAddr + i + (j * 4));
                }
            }
        }
    }

    // Int64 (Qword) 加速：一次比较 4 个 Int64
    static void scanInt64Exact(const uint8_t* buffer, size_t size, int64_t target, uint64_t baseAddr, std::vector<uint64_t>& results) {
        __m256i vTarget = _mm256_set1_epi64x(target);
        for (size_t i = 0; i + 31 < size; i += 32) {
            __m256i vData = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(buffer + i));
            __m256i vMask = _mm256_cmpeq_epi64(vData, vTarget);
            int mask = _mm256_movemask_pd(_mm256_castsi256_pd(vMask));
            if (mask != 0) {
                for (int j = 0; j < 4; ++j) {
                    if (mask & (1 << j)) results.push_back(baseAddr + i + (j * 8));
                }
            }
        }
    }

    // ============================================================
    // 2. 浮点类型精确扫描 (Float, Double)
    // 注意：浮点比较受精度影响，精确扫描通常匹配其二进制表示
    // ============================================================

    static void scanFloatExact(const uint8_t* buffer, size_t size, float target, uint64_t baseAddr, std::vector<uint64_t>& results) {
        __m256 vTarget = _mm256_set1_ps(target);
        for (size_t i = 0; i + 31 < size; i += 32) {
            __m256 vData = _mm256_loadu_ps(reinterpret_cast<const float*>(buffer + i));
            // _CMP_EQ_OQ: 排序且有序比较（处理 NaN）
            __m256 vMask = _mm256_cmp_ps(vData, vTarget, _CMP_EQ_OQ);
            int mask = _mm256_movemask_ps(vMask);
            if (mask != 0) {
                for (int j = 0; j < 8; ++j) {
                    if (mask & (1 << j)) results.push_back(baseAddr + i + (j * 4));
                }
            }
        }
    }

    static void scanDoubleExact(const uint8_t* buffer, size_t size, double target, uint64_t baseAddr, std::vector<uint64_t>& results) {
        __m256d vTarget = _mm256_set1_pd(target);
        for (size_t i = 0; i + 31 < size; i += 32) {
            __m256d vData = _mm256_loadu_pd(reinterpret_cast<const double*>(buffer + i));
            __m256d vMask = _mm256_cmp_pd(vData, vTarget, _CMP_EQ_OQ);
            int mask = _mm256_movemask_pd(vMask);
            if (mask != 0) {
                for (int j = 0; j < 4; ++j) {
                    if (mask & (1 << j)) results.push_back(baseAddr + i + (j * 8));
                }
            }
        }
    }

    // ============================================================
    // 3. 范围扫描加速 (Between) - 超越 CE 的性能关键
    // ============================================================
    static void scanInt32Range(const uint8_t* buffer, size_t size, int32_t min, int32_t max, uint64_t baseAddr, std::vector<uint64_t>& results) {
        __m256i vMin = _mm256_set1_epi32(min - 1); // 用于大于判断
        __m256i vMax = _mm256_set1_epi32(max + 1); // 用于小于判断
        for (size_t i = 0; i + 31 < size; i += 32) {
            __m256i vData = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(buffer + i));
            // (data > min-1) AND (data < max+1)
            __m256i maskGT = _mm256_cmpgt_epi32(vData, vMin);
            __m256i maskLT = _mm256_cmpgt_epi32(vMax, vData);
            __m256i vMask = _mm256_and_si256(maskGT, maskLT);

            int mask = _mm256_movemask_ps(_mm256_castsi256_ps(vMask));
            if (mask != 0) {
                for (int j = 0; j < 8; ++j) {
                    if (mask & (1 << j)) results.push_back(baseAddr + i + (j * 4));
                }
            }
        }
    }


    // 针对 Int32 的 Decreased Value (减少的值) SIMD 比较
    static void scanInt32Decreased(const uint8_t* curBuf, const uint8_t* oldBuf, size_t size,
        uint64_t baseAddr, std::vector<uint64_t>& results) {
        for (size_t i = 0; i + 31 < size; i += 32) {
            __m256i vCur = _mm256_loadu_si256((const __m256i*)(curBuf + i));
            __m256i vOld = _mm256_loadu_si256((const __m256i*)(oldBuf + i));

            // AVX2 没有直接的 epi32 小于指令，使用 cmpgt(old, cur) 相当于 cur < old
            __m256i vMask = _mm256_cmpgt_epi32(vOld, vCur);
            int mask = _mm256_movemask_ps(_mm256_castsi256_ps(vMask));
            if (mask != 0) {
                for (int j = 0; j < 8; ++j) {
                    if (mask & (1 << j)) results.push_back(baseAddr + i + (j * 4));
                }
            }
        }
    }


    // 针对 Int32 的 Increased Value (增加的值) SIMD 比较
    static void scanInt32Increased(const uint8_t* curBuf, const uint8_t* oldBuf, size_t size,
        uint64_t baseAddr, std::vector<uint64_t>& results) {
        for (size_t i = 0; i + 31 < size; i += 32) {
            __m256i vCur = _mm256_loadu_si256((const __m256i*)(curBuf + i));
            __m256i vOld = _mm256_loadu_si256((const __m256i*)(oldBuf + i));
            __m256i vMask = _mm256_cmpgt_epi32(vCur, vOld); // cur > old
            int mask = _mm256_movemask_ps(_mm256_castsi256_ps(vMask));
            if (mask != 0) {
                for (int j = 0; j < 8; ++j) {
                    if (mask & (1 << j)) results.push_back(baseAddr + i + (j * 4));
                }
            }
        }
    }

};