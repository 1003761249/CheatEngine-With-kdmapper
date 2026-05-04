#pragma once
#include <cstdint>
#include <string>

class IMemoryAccessor
{
public:
    virtual ~IMemoryAccessor() = default;

    virtual bool attach(uint32_t pid) = 0;
    virtual void detach() = 0;

    virtual bool read(uint64_t addr, void* buffer, size_t size) = 0;
    virtual bool write(uint64_t addr, const void* buffer, size_t size) = 0;
    virtual void* nativeHandle() = 0;
    virtual std::string name() const = 0;   // ★ 新增
    // ★ 新增：检测目标进程是否存活
    virtual bool isProcessAlive() const = 0;

};