#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <variant>
#include "scan_types.h"
#include "scan_result.h"


// 扫描模式：首次 或 再次
enum class ScanMode { First, Next };

// 数值参数（首次/再次均适用，再次时只需 value1）
struct ValueParams {
    uint64_t value1 = 0;
    uint64_t value2 = 0;   // 仅在 Between 时使用
};

// 字符串参数
struct StringParams {
    std::string text;
    bool caseSensitive = true;
};

// 字节数组 (AOB) 参数
struct AobParams {
    std::vector<uint8_t> pattern;
    std::vector<bool>   mask;
};

// 统一参数体
using ScanParams = std::variant<ValueParams, StringParams, AobParams>;

// 扫描请求（唯一入口）
struct ScanRequest {
    ScanMode    mode = ScanMode::First;
    ScanDataType dataType = ScanDataType::Int64;

    size_t      alignment = 4;
    // 仅在 mode == First 时使用
    ScanType    firstType = ScanType::ExactValue;
    // 仅在 mode == Next 时使用
    NextScanType nextType = NextScanType::Equal;

    // 模块过滤（0 表示不作限制）
    uint64_t moduleBase = 0;
    uint64_t moduleSize = 0;

    // 具体参数（由 mode 和 dataType 决定取哪种）
    ScanParams  params;

    bool onlyWritable;
    bool includeExecutable;

    std::shared_ptr<const std::vector<ScanResult>> prevResults = nullptr;
};