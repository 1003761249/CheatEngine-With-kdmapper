#include "win32_memory_accessor.h"

bool Win32MemoryAccessor::attach(uint32_t pid)
{
    hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    return hProcess != nullptr;
}

void Win32MemoryAccessor::detach()
{
    if (hProcess)
    {
        CloseHandle(hProcess);
        hProcess = nullptr;
    }
}

bool Win32MemoryAccessor::read(uint64_t addr, void* buffer, size_t size)
{
    SIZE_T bytesRead;
    return ReadProcessMemory(hProcess, (LPCVOID)addr, buffer, size, &bytesRead);
}

bool Win32MemoryAccessor::write(uint64_t addr, const void* buffer, size_t size)
{
    SIZE_T bytesWritten;
    return WriteProcessMemory(hProcess, (LPVOID)addr, buffer, size, &bytesWritten);
}

void* Win32MemoryAccessor::nativeHandle()
{
    return hProcess;
}

std::string Win32MemoryAccessor::name() const { return "Win32 API"; }

bool Win32MemoryAccessor::isProcessAlive() const
{
    if (!hProcess)
        return false;
    // 等待 0 毫秒：若返回 WAIT_TIMEOUT 表示进程仍在运行
    DWORD result = WaitForSingleObject(hProcess, 0);
    return (result == WAIT_TIMEOUT);
}