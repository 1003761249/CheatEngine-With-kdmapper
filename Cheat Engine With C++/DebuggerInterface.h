#pragma once
#include <vector>
#include <string>
#include <cstdint>
#ifdef _WIN32 
#include <windows.h>
#endif 
#include "NewKernelHandler.h"



using DWORD = uint32_t;
using BOOL = int;
using THandle = void*;

//struct TContext {};
//struct TARMCONTEXT {};
//struct TARM64CONTEXT {};
//struct TDebugEvent {};

class TDebuggerInterface {
private:
    std::string status;
    TMultiReadExclusiveWriteSynchronizer* statusmrew;
    std::vector<THandle> noBreakList;

    std::string getCurrentDebuggerAttachStatus();
    void setCurrentDebuggerAttachStatus(const std::string& newstatus);

protected:
    int fmaxInstructionBreakpointCount;
    int fmaxWatchpointBreakpointCount;
    int fmaxSharedBreakpointCount;

    enum class TDebuggerCapabilities {
        dbcHardwareBreakpoint,
        dbcSoftwareBreakpoint,
        dbcExceptionBreakpoint,
        dbcDBVMBreakpoint,
        dbcBreakOnEntry,
        dbcCanUseInt1BasedBreakpoints
    };

    using TDebuggerCapabilitiesSet = uint8_t;
    TDebuggerCapabilitiesSet fDebuggerCapabilities;
    std::string fErrorString;

public:
    std::string name;

    TDebuggerInterface();
    virtual ~TDebuggerInterface();

    virtual BOOL WaitForDebugEvent(DEBUG_EVENT* lpDebugEvent, DWORD dwMilliseconds) = 0;
    virtual BOOL ContinueDebugEvent(DWORD dwProcessId, DWORD dwThreadId, DWORD dwContinueStatus) = 0;

    virtual BOOL SetThreadContext(THandle hThread, const CONTEXT& lpContext, bool isFrozenThread = false);
    //virtual BOOL SetThreadContextArm(THandle hThread, const TARMCONTEXT& lpContext, bool isFrozenThread = false);
    //virtual BOOL SetThreadContextArm64(THandle hThread, const TARM64CONTEXT& lpContext, bool isFrozenThread = false);

    virtual BOOL GetThreadContext(THandle hThread, CONTEXT* lpContext, bool isFrozenThread = false);
    //virtual BOOL GetThreadContextArm(THandle hThread, TARMCONTEXT* lpContext, bool isFrozenThread = false);
    //virtual BOOL GetThreadContextArm64(THandle hThread, TARM64CONTEXT* lpContext, bool isFrozenThread = false);

    virtual BOOL DebugActiveProcess(DWORD dwProcessId) = 0;
    virtual BOOL DebugActiveProcessStop(DWORD dwProcessID);
    virtual int GetLastBranchRecords(void* lbr);
    virtual bool isInjectedEvent();
    virtual bool usesDebugRegisters();

    virtual bool inNoBreakList(int threadid);
    virtual void AddToNoBreakList(int threadid);
    virtual void RemoveFromNoBreakList(int threadid);

    virtual bool canUseIPT();
    virtual bool canReportExactDebugRegisterTrigger();
    virtual bool needsToAttach();
    virtual bool controlsTheThreadList();

    TDebuggerCapabilitiesSet DebuggerCapabilities() const { return fDebuggerCapabilities; }
    std::string errorString() const { return fErrorString; }

    int maxInstructionBreakpointCount() const { return fmaxInstructionBreakpointCount; }
    int maxWatchpointBreakpointCount() const { return fmaxWatchpointBreakpointCount; }
    int maxSharedBreakpointCount() const { return fmaxSharedBreakpointCount; }

    std::string debuggerAttachStatus() { return getCurrentDebuggerAttachStatus(); }
    void debuggerAttachStatus(const std::string& s) { setCurrentDebuggerAttachStatus(s); }
};
