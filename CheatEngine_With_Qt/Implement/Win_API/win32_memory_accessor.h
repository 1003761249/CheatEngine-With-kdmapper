#pragma once
#include "imemory_accessor.h"
#include <Windows.h>

class Win32MemoryAccessor : public IMemoryAccessor
{
public:
    bool attach(uint32_t pid) override;
    void detach() override;

    bool read(uint64_t addr, void* buffer, size_t size) override;
    bool write(uint64_t addr, const void* buffer, size_t size) override;
    void* nativeHandle() override;
    std::string name() const override;

    bool isProcessAlive() const override;
private:
    HANDLE hProcess = nullptr;
};