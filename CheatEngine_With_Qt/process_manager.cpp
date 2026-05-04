#include "process_manager.h"
#include "memory_accessor_factory.h"
#include "module_enumerator_factory.h"
#include "memory_region_enumerator_factory.h"
#include "process_enumerator_factory.h"
#include <algorithm>

ProcessManager& ProcessManager::instance()
{
    static ProcessManager inst;
    std::call_once(inst.m_initFlag, []() { inst.initialize(); });
    return inst;
}

bool ProcessManager::isProcessAlive() const
{
    if (m_pid == 0 || !m_accessor)
        return false;

    return m_accessor->isProcessAlive();
}

void ProcessManager::initialize()
{
    createBackends();
}

void ProcessManager::createBackends()
{
    m_accessor = MemoryAccessorFactory::create(m_backend);
    m_moduleEnumerator = ModuleEnumeratorFactory::create(m_backend);
    m_regionEnumerator = MemoryRegionEnumeratorFactory::create(m_backend);
    m_processEnumerator = ProcessEnumeratorFactory::create(m_backend);
}

bool ProcessManager::attach(const ProcessInfo& process)
{
    detach();

    // 重新创建后端（切换后端时可能会调用，但这里保证后端有效）
    if (!m_accessor) createBackends();

    if (m_accessor && m_accessor->attach(process.pid)) {
        m_pid = process.pid;
        updateModules();
        return true;
    }

    m_accessor.reset();
    m_moduleEnumerator.reset();
    m_regionEnumerator.reset();   // 清除可能无效的枚举器

    return false;
}

void ProcessManager::detach()
{
    if (m_accessor)
        m_accessor->detach();


    m_accessor.reset();
    m_moduleEnumerator.reset();
    m_regionEnumerator.reset();   // 注意：进程枚举器不应在 detach 时清除，它不依赖于进程

    m_pid = 0;
    m_modules.clear();
}

std::shared_ptr<IMemoryAccessor> ProcessManager::memory() { return m_accessor; }
IMemoryRegionEnumerator* ProcessManager::regionEnumerator() { return m_regionEnumerator.get(); }
IModuleEnumerator* ProcessManager::moduleEnumerator() { return m_moduleEnumerator.get(); }
IProcessEnumerator* ProcessManager::processEnumerator() { return m_processEnumerator.get(); }


IMemoryAccessor* ProcessManager::memory_naked() {
    return m_accessor.get();  // 增加引用计数
}

void ProcessManager::updateModules()
{
    std::unique_lock lock(m_modulesMutex);    // ★ 写锁
    if (m_moduleEnumerator && m_pid)
        m_modules = m_moduleEnumerator->enumerate(m_pid);
    else
        m_modules.clear();
}

bool ProcessManager::resolveAddress(uint64_t addr, std::string& outDisplay, bool& isBase) const
{
    std::shared_lock lock(m_modulesMutex);    // ★ 读锁
    // 线性查找，模块数量通常不大
    for (const auto& mod : m_modules)
    {
        if (addr >= mod.base && addr < mod.base + mod.size)
        {
            uint64_t offset = addr - mod.base;
            if (offset == 0)
                outDisplay = mod.name;
            else
            {
                // 格式：module.dll+0xHEX（不带0x前缀可自行调整）
                outDisplay = mod.name + "+0x";
                // 手动转为十六进制
                char buf[17];
                snprintf(buf, sizeof(buf), "%llX", offset);
                outDisplay += buf;
            }
            isBase = (offset == 0);
            return true;
        }
    }
    // 未找到模块，返回纯地址十六进制
    char buf[19];
    snprintf(buf, sizeof(buf), "0x%llX", addr);
    outDisplay = buf;
    isBase = false;
    return false;
}


void ProcessManager::setMemoryBackend(MemoryBackend type)
{
    m_backend = type;
    // 如果当前已附加进程，重新连接？
    // 可以要求用户先 detach 或自动重连（这里选择自动重连）
    if (m_pid != 0) {
        // 重新附加当前进程
        ProcessInfo info;
        info.pid = m_pid;
        attach(info);
    }
}

const ModuleInfo* ProcessManager::getModuleByName(const std::string& name) const
{
    std::shared_lock lock(m_modulesMutex);
    for (const auto& mod : m_modules) {
        if (mod.name == name)
            return &mod;
    }
    return nullptr;
}

