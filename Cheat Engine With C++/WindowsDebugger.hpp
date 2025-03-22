#include <Windows.h>
#include <string>
#include <list>
#include "DebuggerInterface.h"
#include "cefuncproc.h"
#include "newkernelhandler.h"
#include "symbolhandler.h"

class TWindowsDebuggerInterface : public TDebuggerInterface {
public:
    // 接口方法声明 
    BOOL WaitForDebugEvent(LPDEBUG_EVENT lpDebugEvent, DWORD dwMilliseconds) override;
    BOOL ContinueDebugEvent(DWORD dwProcessId, DWORD dwThreadId, DWORD dwContinueStatus) override;
    BOOL SetThreadContext(HANDLE hThread, const CONTEXT* lpContext, bool isFrozenThread = false) override;
    BOOL GetThreadContext(HANDLE hThread, CONTEXT* lpContext, bool isFrozenThread = false) override;
    BOOL DebugActiveProcess(DWORD dwProcessId) override;
    BOOL DebugActiveProcessStop(DWORD dwProcessID) override;
    bool canUseIPT() override;

    // 构造函数 
    TWindowsDebuggerInterface();
};