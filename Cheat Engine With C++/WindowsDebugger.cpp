#include "WindowsDebugger.hpp"



TWindowsDebuggerInterface::TWindowsDebuggerInterface() : TDebuggerInterface() {
    fDebuggerCapabilities = fDebuggerCapabilities |
        std::set<DebuggerCapability>{dbcSoftwareBreakpoint,
        dbcHardwareBreakpoint,
        dbcExceptionBreakpoint,
        dbcBreakOnEntry};
    name = "Windows Debugger";
    fmaxSharedBreakpointCount = 4;
}

BOOL TWindowsDebuggerInterface::WaitForDebugEvent(LPDEBUG_EVENT lpDebugEvent, DWORD dwMilliseconds) {
    return ::WaitForDebugEvent(lpDebugEvent, dwMilliseconds);
}

BOOL TWindowsDebuggerInterface::ContinueDebugEvent(DWORD dwProcessId, DWORD dwThreadId, DWORD dwContinueStatus) {
    return ::ContinueDebugEvent(dwProcessId, dwThreadId, dwContinueStatus);
}

BOOL TWindowsDebuggerInterface::SetThreadContext(HANDLE hThread, const CONTEXT* lpContext, bool isFrozenThread) {
    return ::SetThreadContext(hThread, lpContext);
}

BOOL TWindowsDebuggerInterface::GetThreadContext(HANDLE hThread, CONTEXT* lpContext, bool isFrozenThread) {
    return ::GetThreadContext(hThread, lpContext);
}

BOOL TWindowsDebuggerInterface::DebugActiveProcess(DWORD dwProcessId) {
    if (ProcessHandler::processId != dwProcessId) {
        ProcessHandler::processId = dwProcessId;
        OpenProcess();

        SymbolHandler::Reinitialize();
        SymbolHandler::WaitForSymbolsLoaded(true);
    }

    if (PreventDebuggerDetection) {
        std::list<std::string> asmCode = {
            "IsDebuggerPresent:",
            "xor eax, eax",
            "ret"
        };
        try {
            AutoAssemble(asmCode, false);
        }
        catch (...) {
            // 异常处理保持空实现 
        }
    }

    BOOL result = ::DebugActiveProcess(dwProcessId);
    if (!result) {
        ferrorstring = std::format("Error attaching the windows debugger: {}", GetLastError());
    }
    else {
        SymbolHandler::Reinitialize();
    }
    return result;
}

bool TWindowsDebuggerInterface::canUseIPT() {
    return true;
}

BOOL TWindowsDebuggerInterface::DebugActiveProcessStop(DWORD dwProcessID) {
    if (CEDebugger::DebugActiveProcessStop) {
        return CEDebugger::DebugActiveProcessStop(dwProcessID);
    }
    return FALSE;
}

#endif
