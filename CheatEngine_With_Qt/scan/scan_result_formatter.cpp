#pragma once

#include "scan_result_formatter.h"
#include "process_manager.h"
#include <cstring>
#include <cstdio>
#include <cctype>

std::string ScanResultFormatter::formatValue(uint64_t raw, ScanDataType type) {
    char buf[64] = {};
    switch (type) {
    case ScanDataType::Int8:
        snprintf(buf, sizeof(buf), "%d", static_cast<int8_t>(raw));
        break;
    case ScanDataType::Int16:
        snprintf(buf, sizeof(buf), "%d", static_cast<int16_t>(raw));
        break;
    case ScanDataType::Int32:
        snprintf(buf, sizeof(buf), "%d", static_cast<int32_t>(raw));
        break;
    case ScanDataType::Int64:
        snprintf(buf, sizeof(buf), "%lld", static_cast<long long>(raw));
        break;
    case ScanDataType::Float32: {
        float f;
        std::memcpy(&f, &raw, sizeof(f));
        snprintf(buf, sizeof(buf), "%g", f);
        break;
    }
    case ScanDataType::Float64: {
        double d;
        std::memcpy(&d, &raw, sizeof(d));
        snprintf(buf, sizeof(buf), "%g", d);
        break;
    }
    default:
        return std::to_string(raw);
    }
    return std::string(buf);
}

std::string ScanResultFormatter::formatStringAt(uint64_t addr, ScanDataType type) {
    auto mem = ProcessManager::instance().memory();
    if (!mem) return "???";

    if (type == ScanDataType::AsciiString) {
        char buf[64] = {};
        if (!mem->read(addr, buf, sizeof(buf)))
            return "???";
        size_t len = 0;
        while (len < sizeof(buf) && buf[len]) ++len;
        return std::string(buf, len);
    }
    else { // Utf16String
        char16_t buf[32] = {};
        if (!mem->read(addr, buf, sizeof(buf)))
            return "???";
        int len = 0;
        while (len < 32 && buf[len]) ++len;
        // 简单的 UTF‑16 到 UTF‑8 转换（仅支持 BMP）
        std::string utf8;
        for (int i = 0; i < len; ++i) {
            char16_t c = buf[i];
            if (c < 0x80) {
                utf8 += static_cast<char>(c);
            }
            else if (c < 0x800) {
                utf8 += static_cast<char>(0xC0 | (c >> 6));
                utf8 += static_cast<char>(0x80 | (c & 0x3F));
            }
            else {
                utf8 += static_cast<char>(0xE0 | (c >> 12));
                utf8 += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
                utf8 += static_cast<char>(0x80 | (c & 0x3F));
            }
        }
        return utf8;
    }
}

std::string ScanResultFormatter::formatByteArrayAt(uint64_t addr) {
    auto mem = ProcessManager::instance().memory();
    if (!mem) return "???";

    uint8_t buf[32];
    if (!mem->read(addr, buf, sizeof(buf)))
        return "???";

    std::string hex;
    char tmp[4];
    for (size_t i = 0; i < sizeof(buf); ++i) {
        snprintf(tmp, sizeof(tmp), "%02X ", buf[i]);
        hex += tmp;
    }
    return hex;
}