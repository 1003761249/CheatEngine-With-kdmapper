#pragma once
#include <cstdint>

// 抽象出更底层的“总线”读写
class IMemoryTransport {
    virtual bool readRaw(uint64_t physicalAddr, void* buf, size_t size) = 0;
};