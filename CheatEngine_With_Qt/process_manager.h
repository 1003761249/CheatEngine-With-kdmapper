#pragma once
#include <memory>
#include <string>
#include <vector>
#include <mutex> 
#include <shared_mutex>

#include "process_info.h"
#include "module_info.h"

#include "imemory_accessor.h"
#include "imodule_enumerator.h"
#include "imemory_region_enumerator.h"
#include "iprocess_enumerator.h"

#include "memory_accessor_factory.h"

class ProcessManager
{
public:
    static ProcessManager& instance();

    bool attach(const ProcessInfo& process);
    void detach();

    std::shared_ptr<IMemoryAccessor> memory();
    
    IMemoryRegionEnumerator* regionEnumerator();
    IModuleEnumerator* moduleEnumerator();
    IProcessEnumerator* processEnumerator();
    
    bool resolveAddress(uint64_t addr, std::string& outDisplay, bool& isBase) const; // 地址解析：返回模块表示文本与是否为基址
    const std::vector<ModuleInfo>& modules() const { std::shared_lock lock(m_modulesMutex); return m_modules; } // 模块列表
    const ModuleInfo* getModuleByName(const std::string& name) const;


    void setMemoryBackend(MemoryBackend type);   // 切换后端类型
    MemoryBackend currentBackend() const { return m_backend; }

    bool isProcessAlive() const;

private:
    IMemoryAccessor* memory_naked();    // 后台任务使用
    ProcessManager() = default;

    void initialize();
    void updateModules();
    void createBackends();  // 统一创建内存访问器和模块枚举器


    std::unique_ptr<IModuleEnumerator> m_moduleEnumerator;
    std::shared_ptr<IMemoryAccessor> m_accessor;
    std::unique_ptr<IMemoryRegionEnumerator> m_regionEnumerator;
    std::unique_ptr<IProcessEnumerator> m_processEnumerator;

    uint32_t m_pid = 0;
    std::vector<ModuleInfo> m_modules;
    MemoryBackend m_backend = MemoryBackend::Win32;
    std::once_flag m_initFlag;

    mutable std::shared_mutex m_modulesMutex;

};