#pragma once
#include <string>
#include <cstdint>
#include "scan_types.h"

class ScanResultFormatter {
public:
    // 按数据类型格式化数值
    static std::string formatValue(uint64_t raw, ScanDataType type);

    // 从内存读取字符串并格式化（ASCII / UTF‑16）
    static std::string formatStringAt(uint64_t addr, ScanDataType type);

    // 从内存读取字节并格式化为 “42 8B 03 …” 形式
    static std::string formatByteArrayAt(uint64_t addr);

    ScanResultFormatter() = delete;
};