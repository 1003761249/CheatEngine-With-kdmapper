#pragma once
#include <cstdint>

enum class ScanType
{
    ExactValue,
    GreaterThan,
    LessThan,
    Between,
    UnknownInitial,
    StringScan   // ¡ï ÐÂÔö£º×Ö·û´®É¨Ãè
};

enum class NextScanType
{
    Equal,
    NotEqual,
    Increased,
    Decreased,
    Changed,
    Unchanged,
    Between
};

enum class ScanDataType : uint8_t {
    Int8,
    Int16,
    Int32,
    Int64,
    Float32,
    Float64,
    AsciiString,  // ¡ï ASCII / UTF-8 ×Ö·û´®£¨µ¥×Ö½Ú±àÂë£©
    Utf16String,   // ¡ï UTF-16 LE ×Ö·û´®
    ByteArray
};

inline size_t scanDataTypeSize(ScanDataType t) {
    switch (t) {
    case ScanDataType::Int8:    return 1;
    case ScanDataType::Int16:   return 2;
    case ScanDataType::Int32:   return 4;
    case ScanDataType::Int64:   return 8;
    case ScanDataType::Float32: return 4;
    case ScanDataType::Float64: return 8;
    case ScanDataType::AsciiString: return 0; 
    case ScanDataType::Utf16String: return 0;
    case ScanDataType::ByteArray:   return 0;
    default: return 0;
    }
}

inline bool isFloatingPoint(ScanDataType t) {
    return t == ScanDataType::Float32 || t == ScanDataType::Float64;
}

inline bool isStringType(ScanDataType t) {
    return t == ScanDataType::AsciiString || t == ScanDataType::Utf16String;
}

inline bool isByteArrayType(ScanDataType t) {
    return t == ScanDataType::ByteArray;
}