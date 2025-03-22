#pragma once

#define MAX_MODULE_NAME32  255

#include <windows.h>
#include <winternl.h>
#include <psapi.h>
#include <string>
#include "plugin.h"
#include "Filehandler.h"
#include "debug.h"
#include "dbk32functions.h"


using QWORD = uintptr_t;

#pragma pack(push, 1)

using TCONTEXT = CONTEXT;

typedef struct _PROCESSENTRY32W {
	DWORD     dwSize;
	DWORD     cntUsage;
	DWORD     th32ProcessID;
	ULONG_PTR th32DefaultHeapID;
	DWORD     th32ModuleID;
	DWORD     cntThreads;
	DWORD     th32ParentProcessID;
	LONG      pcPriClassBase;
	DWORD     dwFlags;
	char     szExeFile[MAX_PATH];
} PROCESSENTRY32W, * LPPROCESSENTRY32, TProcessEntry32;


typedef struct tagTHREADENTRY32 {
	DWORD dwSize;
	DWORD cntUsage;
	DWORD th32ThreadID;       // this thread
	DWORD th32OwnerProcessID; // Process this thread is associated with
	LONG tpBasePri;
	LONG tpDeltaPri;
	DWORD dwFlags;
} THREADENTRY32, * PTHREADENTRY32, * LPTHREADENTRY32, TThreadEntry32;

typedef struct tagHEAPLIST32 {
	SIZE_T dwSize;
	DWORD th32ProcessID;   // owning process
	ULONG_PTR th32HeapID;  // heap (in owning process's context!)
	DWORD dwFlags;
} HEAPLIST32, * PHEAPLIST32, * LPHEAPLIST32, THeapList32;


typedef struct _MODULEENTRY32W {
	DWORD   dwSize;
	DWORD   th32ModuleID;
	DWORD   th32ProcessID;
	DWORD   GlblcntUsage;
	DWORD   ProccntUsage;
	BYTE* modBaseAddr;
	DWORD   modBaseSize;
	HMODULE hModule;
	char   szModule[MAX_MODULE_NAME32 + 1];
	char   szExePath[MAX_PATH];
} MODULEENTRY32W, * LPMODULEENTRY32, TModuleEntry32;

struct XMM_SAVE_AREA32 {
	unsigned short ControlWord;
	unsigned short StatusWord;
	unsigned char TagWord;
	unsigned char Reserved1;
	unsigned short ErrorOpcode;
	unsigned long ErrorOffset;
	unsigned short ErrorSelector;
	unsigned short Reserved2;
	unsigned long DataOffset;
	unsigned short DataSelector;
	unsigned short Reserved3;
	unsigned long MxCsr;
	unsigned long MxCsr_Mask;
	M128A FloatRegisters[8];
	M128A XmmRegisters[16];
	unsigned char Reserved4[96];
};

typedef XMM_SAVE_AREA32 TXmmSaveArea;
typedef XMM_SAVE_AREA32* PXmmSaveArea;

const auto LEGACY_SAVE_AREA_LENGTH = sizeof(XMM_SAVE_AREA32);

// PSAPI 相关结构体 
struct PSAPI_WORKING_SET_INFORMATION {
	ULONG_PTR NumberOfEntries;
	ULONG_PTR WorkingSetInfo[1]; // 前12位为标志位，后续为地址 
};
using PPSAPI_WORKING_SET_INFORMATION = PSAPI_WORKING_SET_INFORMATION*;

struct PSAPI_WS_WATCH_INFORMATION {
	uintptr_t FaultingPc;
	uintptr_t FaultingVa;
};
using PPSAPI_WS_WATCH_INFORMATION = PSAPI_WS_WATCH_INFORMATION*;

union TJclMMRegister {
	uint8_t mt8Bytes[8];
	uint16_t mt4Words[4];
	uint32_t mt2DWords[2];
	int64_t mt1QWord;
	float mt2Singles[2];
	double mt1Double;
};

union TJclFPUData {
	long double FloatValue; // 注意：可能需要根据实际使用调整精度 
	struct {
		TJclMMRegister MMRegister;
		uint16_t Reserved;
	};
};

struct TJclFPURegister {
	TJclFPUData Data;
	uint8_t Reserved[6];
};
using TJclFPURegisters = TJclFPURegister[8];

union TJclXMMRegister {
	uint8_t xt16Bytes[16];
	uint16_t xt8Words[8];
	uint32_t xt4DWords[4];
	int64_t xt2QWords[2];
	float xt4Singles[4];
	double xt2Doubles[2];
};

struct TJclXMMRegisters {
	union {
		struct {
			TJclXMMRegister LegacyXMM[8];
			uint8_t LegacyReserved[128];
		};
		TJclXMMRegister LongXMM[16];
	};
};

struct TextendedRegisters {
	uint16_t FCW;
	uint16_t FSW;
	uint8_t FTW;
	uint8_t Reserved1;
	uint16_t FOP;
	uint32_t FpuIp;
	uint16_t CS;
	uint16_t Reserved2;
	uint32_t FpuDp;
	uint16_t DS;
	uint16_t Reserved3;
	uint32_t MXCSR;
	uint32_t MXCSRMask;
	TJclFPURegisters FPURegisters;
	TJclXMMRegisters XMMRegisters;
	uint8_t Reserved4[96];
};

typedef struct _FLOATING_SAVE_AREA {
	DWORD   ControlWord;
	DWORD   StatusWord;
	DWORD   TagWord;
	DWORD   ErrorOffset;
	DWORD   ErrorSelector;
	DWORD   DataOffset;
	DWORD   DataSelector;
	BYTE    RegisterArea[80];
	DWORD   Cr0NpxState;
} FLOATING_SAVE_AREA, * PFLOATING_SAVE_AREA;

using TFloatingSaveArea = FLOATING_SAVE_AREA;

struct _CONTEXT32 {
	DWORD ContextFlags;
	DWORD Dr0;
	DWORD Dr1;
	DWORD Dr2;
	DWORD Dr3;
	DWORD Dr6;
	DWORD Dr7;

	TFloatingSaveArea FloatSave;

	DWORD SegGs;
	DWORD SegFs;
	DWORD SegEs;
	DWORD SegDs;

	DWORD Edi;
	DWORD Esi;
	DWORD Ebx;
	DWORD Edx;
	DWORD Ecx;
	DWORD Eax;

	DWORD Ebp;
	DWORD Eip;
	DWORD SegCs;
	DWORD EFlags;
	DWORD Esp;
	DWORD SegSs;

	TXmmSaveArea ext;
};

using CONTEXT32 = _CONTEXT32;
using TContext32 = _CONTEXT32;
using PContext32 = _CONTEXT32*;
using PCONTEXT32 = _CONTEXT32*;


struct TDebuggerState {
	uint64_t threadid;
	uint64_t causedbydbvm;
	uint64_t eflags;
	uint64_t eax;
	uint64_t ebx;
	uint64_t ecx;
	uint64_t edx;
	uint64_t esi;
	uint64_t edi;
	uint64_t ebp;
	uint64_t esp;
	uint64_t eip;
	uint64_t r8;
	uint64_t r9;
	uint64_t r10;
	uint64_t r11;
	uint64_t r12;
	uint64_t r13;
	uint64_t r14;
	uint64_t r15;
	uint64_t cs;
	uint64_t ds;
	uint64_t es;
	uint64_t fs;
	uint64_t gs;
	uint64_t ss;
	uint64_t dr0;
	uint64_t dr1;
	uint64_t dr2;
	uint64_t dr3;
	uint64_t dr6;
	uint64_t dr7;
	TextendedRegisters fxstate;
	uint64_t LBR_Count;
	uint64_t LBR[16];
};

using PDebuggerState = TDebuggerState*;

enum TBreakType {
	bt_OnInstruction = 0,
	bt_OnWrites = 1,
	bt_OnIOAccess = 2,
	bt_OnReadsAndWrites = 3
};

enum TBreakLength {
	bl_1byte = 0,
	bl_2byte = 1,
	bl_8byte = 2, // Only when in 64-bit
	bl_4byte = 3
};

#ifdef __cplusplus
extern "C" {
#endif


	typedef BOOL(WINAPI* TEnumDeviceDrivers)(LPVOID* lpImageBase, DWORD cb, DWORD* lpcbNeeded);
	typedef DWORD(WINAPI* TGetDeviceDriverBaseNameA)(LPVOID ImageBase, LPSTR lpBaseName, DWORD nSize);
	typedef DWORD(WINAPI* TGetDeviceDriverFileName)(LPVOID ImageBase, LPTSTR lpFilename, DWORD nSize);



	typedef SIZE_T(WINAPI* TGetLargePageMinimum)(void);

	typedef BOOL(WINAPI* TWriteProcessMemory64)(HANDLE hProcess, UINT64 BaseAddress, LPCVOID lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesWritten);
	typedef BOOL(WINAPI* TReadProcessMemory64)(HANDLE hProcess, UINT64 lpBaseAddress, LPVOID lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesRead);

	typedef BOOL(WINAPI* TReadProcessMemory)(HANDLE hProcess, LPCVOID lpBaseAddress, LPVOID lpBuffer, DWORD nSize, SIZE_T* lpNumberOfBytesRead);
	typedef BOOL(WINAPI* TWriteProcessMemory)(HANDLE hProcess, LPCVOID lpBaseAddress, LPCVOID lpBuffer, DWORD nSize, SIZE_T* lpNumberOfBytesWritten);
	typedef BOOL(WINAPI* TGetThreadContext)(HANDLE hThread, LPCONTEXT lpContext);
	typedef BOOL(WINAPI* TSetThreadContext)(HANDLE hThread, const CONTEXT* lpContext);
	typedef HANDLE(WINAPI* TOpenProcess)(DWORD dwDesiredAccess, BOOL bInheritHandle, DWORD dwProcessId);


	typedef BOOL(WINAPI* TWow64GetThreadContext)(HANDLE hThread, PCONTEXT32 lpContext);
	typedef BOOL(WINAPI* TWow64SetThreadContext)(HANDLE hThread, const CONTEXT32* lpContext);
	typedef BOOL(WINAPI* TGetThreadSelectorEntry)(HANDLE hThread, DWORD dwSelector, LDT_ENTRY* lpSelectorEntry);
	typedef DWORD(WINAPI* TSuspendThread)(HANDLE hThread);
	typedef DWORD(WINAPI* TResumeThread)(HANDLE hThread);

	typedef HANDLE(WINAPI* TCreateToolhelp32Snapshot)(DWORD dwFlags, DWORD th32ProcessID);
	typedef BOOL(WINAPI* TProcess32First)(HANDLE hSnapshot, LPPROCESSENTRY32 lppe);
	typedef BOOL(WINAPI* TProcess32Next)(HANDLE hSnapshot, LPPROCESSENTRY32 lppe);
	typedef BOOL(WINAPI* TThread32First)(HANDLE hSnapshot, LPTHREADENTRY32 lpte);
	typedef BOOL(WINAPI* TThread32Next)(HANDLE hSnapshot, LPTHREADENTRY32 lpte);
	typedef BOOL(WINAPI* TModule32First)(HANDLE hSnapshot, LPMODULEENTRY32 lpme);
	typedef BOOL(WINAPI* TModule32Next)(HANDLE hSnapshot, LPMODULEENTRY32 lpme);

	typedef BOOL(WINAPI* THeap32ListFirst)(HANDLE hSnapshot, LPHEAPLIST32 lphl);
	typedef BOOL(WINAPI* THeap32ListNext)(HANDLE hSnapshot, LPHEAPLIST32 lphl);
	typedef BOOL(WINAPI* TIsWow64Process)(HANDLE processhandle, PBOOL isWow);

	typedef BOOL(WINAPI* TWaitForDebugEvent)(LPDEBUG_EVENT lpDebugEvent, DWORD dwMilliseconds);
	typedef BOOL(WINAPI* TContinueDebugEvent)(DWORD dwProcessId, DWORD dwThreadId, DWORD dwContinueStatus);
	typedef BOOL(WINAPI* TDebugActiveProcess)(DWORD dwProcessId);

	typedef BOOL(WINAPI* TVirtualProtect)(LPVOID lpAddress, SIZE_T dwSize, DWORD flNewProtect, PDWORD lpflOldProtect);

	typedef BOOL(WINAPI* TVirtualFreeEx)(HANDLE hProcess, LPVOID lpAddress, SIZE_T dwSize, DWORD dwFreeType);
	typedef BOOL(WINAPI* TVirtualProtectEx)(HANDLE hProcess, LPVOID lpAddress, SIZE_T dwSize, DWORD flNewProtect, PDWORD lpflOldProtect);
	typedef DWORD(WINAPI* TVirtualQueryEx)(HANDLE hProcess, LPCVOID lpAddress, PMEMORY_BASIC_INFORMATION lpBuffer, SIZE_T dwLength);

	typedef LPVOID(WINAPI* TVirtualAllocEx)(HANDLE hProcess, LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect);
	typedef HANDLE(WINAPI* TCreateRemoteThread)(HANDLE hProcess, LPSECURITY_ATTRIBUTES lpThreadAttributes, SIZE_T dwStackSize, LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags, LPDWORD lpThreadId);

	typedef HANDLE(WINAPI* TOpenThread)(DWORD dwDesiredAccess, BOOL bInheritHandle, DWORD dwThreadId);
	typedef UINT64(WINAPI* TGetPEProcess)(DWORD ProcessID);
	typedef UINT64(WINAPI* TGetPEThread)(DWORD Threadid);
	typedef DWORD(WINAPI* TGetDebugportOffset)(void);
	typedef DWORD(WINAPI* TGetThreadsProcessOffset)(void);
	typedef DWORD(WINAPI* TGetThreadListEntryOffset)(void);

	typedef BOOL(WINAPI* TGetPhysicalAddress)(HANDLE hProcess, LPCVOID lpBaseAddress, PUINT64 Address);
	typedef UINT_PTR(WINAPI* TGetCR4)(void);
	typedef BOOL(WINAPI* TGetCR3)(HANDLE hProcess, PUINT64 CR3);
	typedef BOOL(WINAPI* TSetCR3)(HANDLE hProcess, UINT_PTR CR3);
	typedef UINT_PTR(WINAPI* TGetCR0)(void);
	typedef UINT_PTR(WINAPI* TGetSDT)(void);
	typedef UINT_PTR(WINAPI* TGetSDTShadow)(void);

	typedef HANDLE(WINAPI* TCreateRemoteAPC)(DWORD threadid, LPVOID lpStartAddress);

	typedef BOOL(WINAPI* TGetProcessNameFromID)(DWORD processid, LPSTR buffer, DWORD buffersize);
	typedef BOOL(WINAPI* TGetProcessNameFromPEProcess)(UINT64 peprocess, LPSTR buffer, DWORD buffersize);

	typedef BOOL(WINAPI* TStartProcessWatch)(void);
	typedef DWORD(WINAPI* TWaitForProcessListData)(LPVOID processpointer, LPVOID threadpointer, DWORD timeout);

	typedef BOOL(WINAPI* TIsValidHandle)(HANDLE hProcess);
	typedef DWORD(WINAPI* TGetIDTCurrentThread)(void);
	typedef DWORD(WINAPI* TGetIDTs)(PVOID idtstore, int maxidts);
	typedef BOOL(WINAPI* TMakeWritable)(DWORD Address, DWORD Size, BOOL copyonwrite);
	typedef BOOL(WINAPI* TGetLoadedState)(void);

	typedef BOOL(WINAPI* TDBKSuspendThread)(DWORD ThreadID);
	typedef BOOL(WINAPI* TDBKResumeThread)(DWORD ThreadID);
	typedef BOOL(WINAPI* TDBKSuspendProcess)(DWORD ProcessID);
	typedef BOOL(WINAPI* TDBKResumeProcess)(DWORD ProcessID);

	typedef LPVOID(WINAPI* TKernelAlloc)(DWORD size);
	typedef UINT64(WINAPI* TKernelAlloc64)(DWORD size);
	typedef LPVOID(WINAPI* TGetKProcAddress)(LPCWSTR s);
	typedef UINT64(WINAPI* TGetKProcAddress64)(LPCWSTR s);

	typedef BOOL(WINAPI* TGetSDTEntry)(int nr, PDWORD address, PBYTE paramcount);
	typedef BOOL(WINAPI* TGetSSDTEntry)(int nr, PDWORD address, PBYTE paramcount);
	typedef DWORD(WINAPI* TGetGDT)(PWORD limit);

	typedef BOOL(WINAPI* TisDriverLoaded)(PBOOL SigningIsTheCause);
	typedef VOID(WINAPI* TLaunchDBVM)(int cpuid);

	typedef BOOL(WINAPI* TDBKDebug_ContinueDebugEvent)(BOOL handled);
	typedef BOOL(WINAPI* TDBKDebug_WaitForDebugEvent)(DWORD timeout);
	typedef BOOL(WINAPI* TDBKDebug_GetDebuggerState)(PDebuggerState state);
	typedef BOOL(WINAPI* TDBKDebug_SetDebuggerState)(PDebuggerState state);
	typedef BOOL(WINAPI* TDBKDebug_SetGlobalDebugState)(BOOL state);
	typedef BOOL(WINAPI* TDBKDebug_SetAbilityToStepKernelCode)(BOOL state);
	typedef BOOL(WINAPI* TDBKDebug_StartDebugging)(DWORD processid);
	typedef BOOL(WINAPI* TDBKDebug_StopDebugging)(void);
	typedef BOOL(WINAPI* TDBKDebug_GD_SetBreakpoint)(BOOL active, int debugregspot, UINT_PTR Address, TBreakType breakType, TBreakLength breakLength);

	//-------------------------------------DBVM---------------------------------

	typedef DWORD(WINAPI* Tdbvm_version)(void);
	typedef DWORD(WINAPI* Tdbvm_changeselectors)(DWORD cs, DWORD ss, DWORD ds, DWORD es, DWORD fs, DWORD gs);
	typedef DWORD(WINAPI* Tdbvm_restore_interrupts)(void);
	typedef DWORD(WINAPI* Tdbvm_block_interrupts)(void);
	typedef DWORD(WINAPI* Tdbvm_raise_privilege)(void);

	typedef DWORD(WINAPI* Tdbvm_read_physical_memory)(UINT64 PhysicalAddress, LPVOID destination, int size);
	typedef DWORD(WINAPI* Tdbvm_write_physical_memory)(UINT64 PhysicalAddress, LPCVOID source, int size);

	typedef BOOL(WINAPI* TVirtualQueryEx_StartCache)(HANDLE hProcess, DWORD flags);
	typedef void (WINAPI* TVirtualQueryEx_EndCache)(HANDLE hProcess);

	// 以上全部翻译完毕  
	typedef BOOL(WINAPI* TQueryWorkingSet)(HANDLE hProcess, PVOID pv, DWORD cb);
	typedef BOOL(WINAPI* TEmptyWorkingSet)(HANDLE hProcess);
	typedef BOOL(WINAPI* TInitializeProcessForWsWatch)(HANDLE hProcess);
	typedef BOOL(WINAPI* TGetWsChanges)(HANDLE hProcess, PPSAPI_WS_WATCH_INFORMATION lpWatchInfo, DWORD cb);


	typedef BOOL(WINAPI* TCloseHandle)(HANDLE hObject);
	typedef BOOL(WINAPI* TGetLogicalProcessorInformation)(PSYSTEM_LOGICAL_PROCESSOR_INFORMATION Buffer, PDWORD ReturnedLength);
	typedef BOOL(WINAPI* TPrintWindow)(HWND hwnd, HDC hdcBlt, UINT nFlags);
	typedef BOOL(WINAPI* TChangeWindowMessageFilter)(UINT msg, DWORD Action);
	//typedef MEMORY_BASIC_INFORMATION TMemoryBasicInformation;

	typedef DWORD(WINAPI* TGetRegionInfo)(HANDLE hProcess, void* lpAddress, PMEMORY_BASIC_INFORMATION lpBuffer, DWORD dwLength, std::string& mapsline);

	typedef BOOL(WINAPI* TSetProcessDEPPolicy)(DWORD dwFlags);
	typedef BOOL(WINAPI* TGetProcessDEPPolicy)(HANDLE h, PDWORD dwFlags, PBOOL permanent);
	typedef BOOL(WINAPI* TGetProcessId)(HANDLE h);

#ifdef __cplusplus
}
#endif

#pragma pack(pop)


TEnumDeviceDrivers EnumDeviceDrivers;
TGetDeviceDriverBaseNameA GetDeviceDriverBaseNameA;
TGetDeviceDriverFileName GetDeviceDriverFileName;

TQueryWorkingSet QueryWorkingSet;
TEmptyWorkingSet EmptyWorkingSet;
TInitializeProcessForWsWatch InitializeProcessForWsWatch;
TGetWsChanges GetWsChanges;

#ifdef _WIN64  
TReadProcessMemory64 ReadProcessMemory64;
#endif  
TReadProcessMemory ReadProcessMemoryActual;
TWriteProcessMemory WriteProcessMemoryActual;

void* defaultRPM;
void* defaultWPM;

TGetThreadContext GetThreadContext;
TSetThreadContext SetThreadContext;
TOpenProcess OpenProcess;

#ifdef _WIN64  
TWow64GetThreadContext Wow64GetThreadContext;
TWow64SetThreadContext Wow64SetThreadContext;
TGetThreadSelectorEntry GetThreadSelectorEntry;
TSuspendThread SuspendThread;
TResumeThread ResumeThread;
#endif  
TCreateToolhelp32Snapshot CreateToolhelp32Snapshot;

TProcess32First Process32First;
TProcess32Next Process32Next;

TThread32First Thread32First;
TThread32Next Thread32Next;
TModule32First Module32First;
TModule32Next Module32Next;

#ifdef _WIN32  
THeap32ListFirst Heap32ListFirst;
THeap32ListNext Heap32ListNext;



TWaitForDebugEvent WaitForDebugEvent;
TContinueDebugEvent ContinueDebugEvent;
TDebugActiveProcess DebugActiveProcess;

TGetLargePageMinimum GetLargePageMinimum;
TVirtualProtect VirtualProtect;
#endif  
TVirtualProtectEx VirtualProtectEx;
TVirtualQueryEx VirtualQueryExActual;
TVirtualAllocEx VirtualAllocEx;
TVirtualFreeEx VirtualFreeEx;
TCreateRemoteThread CreateRemoteThread;

TOpenThread OpenThread;
TGetThreadsProcessOffset GetThreadsProcessOffset;
TGetThreadListEntryOffset GetThreadListEntryOffset;
TGetDebugportOffset GetDebugportOffset;
TGetPhysicalAddress GetPhysicalAddress;
TGetCR4 GetCR4;
TGetCR3 GetCR3;
TGetCR0 GetCR0;
TGetSDT GetSDT;
TGetSDTShadow GetSDTShadow;
TStartProcessWatch StartProcessWatch;
TWaitForProcessListData WaitForProcessListData;
TGetProcessNameFromID GetProcessNameFromID;
TGetProcessNameFromPEProcess GetProcessNameFromPEProcess;
TIsValidHandle IsValidHandle;
TGetIDTCurrentThread GetIDTCurrentThread;
TGetIDTs GetIDTs;
TMakeWritable MakeWritable;
TGetLoadedState GetLoadedState;
TDBKSuspendThread DBKSuspendThread;
TDBKResumeThread DBKResumeThread;
TDBKSuspendProcess DBKSuspendProcess;
TDBKResumeProcess DBKResumeProcess;
TKernelAlloc KernelAlloc;
TKernelAlloc64 KernelAlloc64;
TGetKProcAddress GetKProcAddress;
TGetKProcAddress64 GetKProcAddress64;
TGetSDTEntry GetSDTEntry;
TGetSSDTEntry GetSSDTEntry;
TisDriverLoaded isDriverLoaded;
TLaunchDBVM LaunchDBVM;
TReadProcessMemory ReadPhysicalMemory;
TWriteProcessMemory WritePhysicalMemory;
TCreateRemoteAPC CreateRemoteAPC;
TGetGDT GetGDT;
TDBKDebug_ContinueDebugEvent DBKDebug_ContinueDebugEvent;
TDBKDebug_WaitForDebugEvent DBKDebug_WaitForDebugEvent;
TDBKDebug_GetDebuggerState DBKDebug_GetDebuggerState;
TDBKDebug_SetDebuggerState DBKDebug_SetDebuggerState;
TDBKDebug_SetGlobalDebugState DBKDebug_SetGlobalDebugState;
TDBKDebug_SetAbilityToStepKernelCode DBKDebug_SetAbilityToStepKernelCode;
TDBKDebug_StartDebugging DBKDebug_StartDebugging;
TDBKDebug_StopDebugging DBKDebug_StopDebugging;
TDBKDebug_GD_SetBreakpoint DBKDebug_GD_SetBreakpoint;

TCloseHandle CloseHandle;
TGetLogicalProcessorInformation GetLogicalProcessorInformation;
TPrintWindow PrintWindow;
TChangeWindowMessageFilter ChangeWindowMessageFilter;


HANDLE WindowsKernel;
HANDLE NTDLLHandle;



class NewKernelHandler
{
public:
	

	//DarkByteKernel: Thandle;
	bool DBKLoaded;

	bool Usephysical;
	bool UseFileAsMemory;
	bool Usephysicaldbvm;
	bool Usedbkquery;
	bool DBKReadWrite;

	bool DenyList;
	bool DenyListGlobal;
	int ModuleListSize;
	void* ModuleList;

	uint8_t MAXPHYADDR; //number of bits a physical address can be made up of
	uintptr_t MAXPHYADDRMASK; //mask to AND with a physical address to strip invalid bits



	//same as MAXPHYADDRMASK but also aligns it to a page boundary
	uintptr_t MAXPHYADDRMASKPBBIG;

	uint8_t MAXLINEARADDR; //number of bits in a virtual address. All bits after it have to match


	const uint64_t MAXLINEARADDRMASK = 0xFFFFFFFFFFFFF000;
	const uint64_t MAXLINEARADDRTEST = 0x0000000000000FFF;
	const uint64_t MAXPHYADDRMASKPB = 0x000FFFFFFFFFF000;



#ifdef _WIN32

	void DONTUseDBKQueryMemoryRegion();
	void DONTUseDBKReadWriteMemory();
	void DONTUseDBKOpenProcess();
	void UseDBKQueryMemoryRegion();
	void UseDBKReadWriteMemory();
	void UseDBKOpenProcess();

#endif

	void DBKFileAsMemory(const std::string& fn, uintptr_t* baseaddress = 0); // overload
	void DBKFileAsMemory(); // overload

#ifdef _WIN32
	DWORD VirtualQueryExPhysical(HANDLE hProcess, LPCVOID lpAddress, PMEMORY_BASIC_INFORMATION lpBuffer, DWORD dwLength);
	void DBKPhysicalMemory();
	void DBKPhysicalMemoryDBVM();
	void DBKProcessMemory();
	void LoadDBK32();

	void OutputDebugString(const std::string& msg);

	void NeedsDBVM(const std::string& Reason = "");

#endif

	BOOL loaddbvmifneeded(const std::string& reason = "");
	BOOL IsRunningDBVM();
	BOOL isDBVMCapable();
	BOOL hasEPTSupport();

#ifdef _WIN32

	BOOL isIntel();
	BOOL isAMD();

#endif

	BOOL Is64bitOS();
	BOOL Is64BitProcess(HANDLE processhandle);

	BOOL WriteProcessMemory(HANDLE hProcess, LPVOID lpBaseAddress, LPCVOID lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesWritten);
	BOOL ReadProcessMemory(HANDLE hProcess, LPCVOID lpBaseAddress, LPVOID lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesRead);
	DWORD VirtualQueryEx(HANDLE hProcess, LPCVOID lpAddress, PMEMORY_BASIC_INFORMATION lpBuffer, SIZE_T dwLength);

#ifdef _WIN32

	BOOL VirtualToPhysicalCR3(QWORD cr3, QWORD VirtualAddress, QWORD* PhysicalAddress);
	BOOL ReadProcessMemoryCR3(QWORD cr3, LPCVOID lpBaseAddress, LPVOID lpBuffer, DWORD nSize, SIZE_T* lpNumberOfBytesRead);
	BOOL WriteProcessMemoryCR3(QWORD cr3, LPCVOID lpBaseAddress, LPCVOID lpBuffer, DWORD nSize, SIZE_T* lpNumberOfBytesWritten);
	DWORD VirtualQueryExCR3(QWORD cr3, LPCVOID lpAddress, PMEMORY_BASIC_INFORMATION lpBuffer, DWORD dwLength);

	BOOL GetPageInfoCR3(QWORD cr3, uintptr_t VirtualAddress, PMEMORY_BASIC_INFORMATION lpBuffer);
	BOOL GetNextReadablePageCR3(QWORD cr3, uintptr_t VirtualAddress, uintptr_t* ReadableVirtualAddress);
#endif


	NewKernelHandler();
	~NewKernelHandler();

	FileHandler* filehandler;
	DBK32functions* dbk32functions;
	plugin* pluginhandler;
	Debug* debug;

};

NewKernelHandler::NewKernelHandler()
{
	pluginhandler = new plugin();
	filehandler = new FileHandler();
	dbk32functions = new DBK32functions();
	debug = new Debug();

}

NewKernelHandler::~NewKernelHandler()
{
}

