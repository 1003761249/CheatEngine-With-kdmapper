#include "NewKernelHandler.h"
#include "frmEditHistoryUnit.h"
#include <cstdint>
#include <algorithm>
#include <windows.h>
#include "Globals.h"
#include <wow64apiset.h>
#include "dbk32/dbk32functions.h"
#include "dbk32/vmxfunction.h"
#include "Filehandler.h"
#include <fstream>
#include "plugin.h"
#include "dbk32functions.h"
using namespace NewKernelHandler;



const char* rsToUseThisFunctionYouWillNeedToRunDBVM = "To use this function you will need to run DBVM. There is a high chance running DBVM can crash your system and make you lose your data(So don't forget to save first). Do you want to run DBVM?";
const char* rsDidNotLoadDBVM = "I don't know what you did, you didn't crash, but you also didn't load DBVM";
const char* rsPleaseRebootAndPressF8BeforeWindowsBoots = "Please reboot and press f8 before windows boots. Then enable unsigned drivers. Alternatively, you could buy yourself a business class certificate and sign the driver yourself (or try debug signing)";
const char* rsTheDriverNeedsToBeLoadedToBeAbleToUseThisFunction = "The driver needs to be loaded to be able to use this function.";
const char* rsYourCpuMustBeAbleToRunDbvmToUseThisFunction = "Your cpu must be able to run dbvm to use this function";
const char* rsCouldnTBeOpened = "%s couldn't be opened";
const char* rsDBVMIsNotLoadedThisFeatureIsNotUsable = "DBVM is not loaded. This feature is not usable";

bool verifyAddress(uint64_t a) {
	bool result = false;
	if ((a & MAXLINEARADDRTEST) > 0) {
		// bits MAXLINEARADDR to 64 need to be 1
		result = (a & MAXLINEARADDRMASK) == MAXLINEARADDRMASK;
	}
	else {
		// bits MAXLINEARADDR to 64 need to be 0
		result = (a & MAXLINEARADDRMASK) == 0;
	}
	return result;
}

uint32_t pageEntryToProtection(uint64_t entry) {
	bool r = (entry & 1) == 1; // present
	bool w = (entry & 2) == 2; // writable
	bool x = !((entry & (uint64_t(1) << 63)) == (uint64_t(1) << 63)); // no execute

	if (!r) return PAGE_NOACCESS;
	if (w) {
		if (x) return PAGE_EXECUTE_READWRITE;
		else return PAGE_READWRITE;
	}
	else {
		if (x) return PAGE_EXECUTE_READ;
		else return PAGE_READONLY;
	}
}

bool GetNextReadablePageCR3(uint64_t cr3, uintptr_t VirtualAddress, uintptr_t& ReadableVirtualAddress) {
	int pml4index;
	int pagedirptrindex;
	int pagedirindex;
	int pagetableindex;
	int offset;

	uint64_t pml4entry;
	uint64_t pagedirptrentry;
	uint64_t pagedirentry;
	uint64_t pagetableentry;

	uint64_t pml4page[512];
	uint64_t pagedirptrpage[512];
	uint64_t pagedirpage[512];
	uint64_t pagetablepage[512];

	uintptr_t x;
	bool found = false;

	cr3 = cr3 & MAXPHYADDRMASKPB;

	bool result = false;
	pml4index = (VirtualAddress >> 39) & 0x1ff;
	pagedirptrindex = (VirtualAddress >> 30) & 0x1ff;
	pagedirindex = (VirtualAddress >> 21) & 0x1ff;
	pagetableindex = (VirtualAddress >> 12) & 0x1ff;

	if (ReadPhysicalMemory(0, reinterpret_cast<void*>(cr3), pml4page, 4096, x)) {
		while (pml4index < 512) {
			if ((pml4page[pml4index] & 1) == 1) {
				x = pml4page[pml4index] & MAXPHYADDRMASKPB;
				if (ReadPhysicalMemory(0, reinterpret_cast<void*>(x), pagedirptrpage, 4096, x)) {
					while (pagedirptrindex < 512) {
						if ((pagedirptrpage[pagedirptrindex] & 1) == 1) {
							x = pagedirptrpage[pagedirptrindex] & MAXPHYADDRMASKPB;
							if (ReadPhysicalMemory(0, reinterpret_cast<void*>(x), pagedirpage, 4096, x)) {
								while (pagedirindex < 512) {
									if ((pagedirpage[pagedirindex] & 1) == 1) {
										// present
										if ((pagedirpage[pagedirindex] & (1 << 7)) == (1 << 7)) {
											if ((pml4index & (1 << 8)) == (1 << 8))
												pml4index = pml4index | (0xffffffffffffffff << 8);

											ReadableVirtualAddress = (static_cast<uint64_t>(pagedirindex) << 21) + (static_cast<uint64_t>(pagedirptrindex) << 30) + (static_cast<uint64_t>(pml4index) << 39);
											return true;
										}
										else {
											// normal pagedir, go through the pagetables
											x = pagedirpage[pagedirindex] & MAXPHYADDRMASKPB;
											if (ReadPhysicalMemory(0, reinterpret_cast<void*>(x), pagetablepage, 4096, x)) {
												while (pagetableindex < 512) {
													if ((pagetablepage[pagetableindex] & 1) == 1) {
														if ((pml4index & (1 << 8)) == (1 << 8))
															pml4index = pml4index | (0xffffffffffffffff << 8);

														ReadableVirtualAddress = (static_cast<uint64_t>(pagetableindex) << 12) + (static_cast<uint64_t>(pagedirindex) << 21) + (static_cast<uint64_t>(pagedirptrindex) << 30) + (static_cast<uint64_t>(pml4index) << 39);
														return true;
													}
													++pagetableindex;
												}
											}
										}
									}
									pagetableindex = 0;
									++pagedirindex;
								}
							}
						}
						pagedirindex = 0;
						pagetableindex = 0;
						++pagedirptrindex;
					}
				}
			}
			pagedirptrindex = 0;
			pagedirindex = 0;
			pagetableindex = 0;
			++pml4index;
		}
	}
	return false;
}




bool GetPageInfoCR3(uint64_t cr3, uintptr_t VirtualAddress, TMemoryBasicInformation& lpBuffer) {
	int pml4index;
	int pagedirptrindex;
	int pagedirindex;
	int pagetableindex;
	int offset;

	uint64_t pml4entry;
	uint64_t pagedirptrentry;
	uint64_t pagedirentry;
	uint64_t pagetableentry;

	uintptr_t x;
	cr3 = cr3 & MAXPHYADDRMASKPB;

	lpBuffer.BaseAddress = reinterpret_cast<void*>(VirtualAddress & 0xfffffffffffff000);
	bool result = false;
	pml4index = (VirtualAddress >> 39) & 0x1ff;
	pagedirptrindex = (VirtualAddress >> 30) & 0x1ff;
	pagedirindex = (VirtualAddress >> 21) & 0x1ff;
	pagetableindex = (VirtualAddress >> 12) & 0x1ff;
	offset = VirtualAddress & 0xfff;

	if (ReadPhysicalMemory(0, reinterpret_cast<void*>(cr3 + pml4index * 8), &pml4entry, 8, x)) {
		if ((pml4entry & 1) == 1) {
			pml4entry = pml4entry & MAXPHYADDRMASKPB;
			if (ReadPhysicalMemory(0, reinterpret_cast<void*>(pml4entry + pagedirptrindex * 8), &pagedirptrentry, 8, x)) {
				if ((pagedirptrentry & 1) == 1) {
					pagedirptrentry = pagedirptrentry & MAXPHYADDRMASKPB;
					if (ReadPhysicalMemory(0, reinterpret_cast<void*>(pagedirptrentry + pagedirindex * 8), &pagedirentry, 8, x)) {
						if ((pagedirentry & 1) == 1) {
							if ((pagedirentry & (1 << 7)) > 0) {  // PS==1
								lpBuffer.AllocationBase = reinterpret_cast<void*>(VirtualAddress & 0xffffffe00000);
								lpBuffer.AllocationProtect = PAGE_EXECUTE_READWRITE;
								lpBuffer.Protect = pageEntryToProtection(pagedirentry);
								lpBuffer.RegionSize = 0x200000 - (reinterpret_cast<uintptr_t>(lpBuffer.BaseAddress) - reinterpret_cast<uintptr_t>(lpBuffer.AllocationBase));
								lpBuffer.State = MEM_COMMIT;
								lpBuffer.Type = MEM_PRIVATE;
								return true;
							}
							else {
								// has pagetable
								pagedirentry = pagedirentry & MAXPHYADDRMASKPB;
								if (ReadPhysicalMemory(0, reinterpret_cast<void*>(pagedirentry + pagetableindex * 8), &pagetableentry, 8, x)) {
									if ((pagetableentry & 1) == 1) {
										lpBuffer.AllocationBase = reinterpret_cast<void*>(VirtualAddress & 0xfffffffffffff000);
										lpBuffer.AllocationProtect = PAGE_EXECUTE_READWRITE;
										lpBuffer.Protect = pageEntryToProtection(pagetableentry);
										lpBuffer.RegionSize = 4096;
										lpBuffer.State = MEM_COMMIT;
										lpBuffer.Type = MEM_PRIVATE;
										return true;
									}
								}
							}
						}
					}
				}
			}
		}
	}
	return false;
}


bool VirtualToPhysicalCR3(uint64_t cr3, uint64_t VirtualAddress, uint64_t& PhysicalAddress) {
	int pml4index;
	int pagedirptrindex;
	int pagedirindex;
	int pagetableindex;
	int offset;

	uint64_t pml4entry;
	uint64_t pagedirptrentry;
	uint64_t pagedirentry;
	uint64_t pagetableentry;

	uintptr_t x;
	cr3 = cr3 & MAXPHYADDRMASKPB;

	bool result = false;
	pml4index = (VirtualAddress >> 39) & 0x1ff;
	pagedirptrindex = (VirtualAddress >> 30) & 0x1ff;
	pagedirindex = (VirtualAddress >> 21) & 0x1ff;
	pagetableindex = (VirtualAddress >> 12) & 0x1ff;
	offset = VirtualAddress & 0xfff;

	if (ReadPhysicalMemory(0, reinterpret_cast<void*>(cr3 + pml4index * 8), &pml4entry, 8, x)) {
		if ((pml4entry & 1) == 1) {
			pml4entry = pml4entry & MAXPHYADDRMASKPB;
			if (ReadPhysicalMemory(0, reinterpret_cast<void*>(pml4entry + pagedirptrindex * 8), &pagedirptrentry, 8, x)) {
				if ((pagedirptrentry & 1) == 1) {
					pagedirptrentry = pagedirptrentry & MAXPHYADDRMASKPB;
					if (ReadPhysicalMemory(0, reinterpret_cast<void*>(pagedirptrentry + pagedirindex * 8), &pagedirentry, 8, x)) {
						if ((pagedirentry & 1) == 1) {
							if ((pagedirentry & (1 << 7)) > 0) {  // PS==1
								PhysicalAddress = (pagedirentry & MAXPHYADDRMASKPB) + (VirtualAddress & 0x1fffff);
								return true;
							}
							else {
								// has pagetable
								pagedirentry = pagedirentry & MAXPHYADDRMASKPB;
								if (ReadPhysicalMemory(0, reinterpret_cast<void*>(pagedirentry + pagetableindex * 8), &pagetableentry, 8, x)) {
									if ((pagetableentry & 1) == 1) {
										PhysicalAddress = (pagetableentry & MAXPHYADDRMASKPB) + offset;
										return true;
									}
								}
							}
						}
					}
				}
			}
		}
	}
	return false;
}

DWORD VirtualQueryExCR3(uint64_t cr3, void* lpAddress, PMEMORY_BASIC_INFORMATION lpBuffer, DWORD dwLength) {
	MEMORY_BASIC_INFORMATION currentmbi;
	uintptr_t nextreadable;
	uintptr_t p;

	if (!verifyAddress(reinterpret_cast<uint64_t>(lpAddress)))
		return 0;

	lpBuffer->BaseAddress = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(lpAddress) & 0xfffffffffffff000);

	if (GetPageInfoCR3(cr3, reinterpret_cast<uintptr_t>(lpAddress), lpBuffer)) {
		p = reinterpret_cast<uintptr_t>(lpBuffer->BaseAddress) + lpBuffer->RegionSize;

		while (lpBuffer->RegionSize < 0x20000000) {
			if (GetPageInfoCR3(cr3, p, currentmbi)) {
				if (currentmbi.Protect == lpBuffer->Protect) {
					p = reinterpret_cast<uintptr_t>(currentmbi.BaseAddress) + currentmbi.RegionSize;
					lpBuffer->RegionSize = p - reinterpret_cast<uintptr_t>(lpBuffer->BaseAddress);
				}
				else {
					return dwLength; // different protection
				}
			}
			else {
				return dwLength; // unreadable
			}
		}

		lpBuffer->RegionSize = p - reinterpret_cast<uintptr_t>(lpBuffer->BaseAddress);
	}
	else {
		// unreadable
		if (GetNextReadablePageCR3(cr3, reinterpret_cast<uintptr_t>(lpAddress), nextreadable)) {
			lpBuffer->AllocationBase = lpBuffer->BaseAddress;
			lpBuffer->RegionSize = nextreadable - reinterpret_cast<uintptr_t>(lpBuffer->BaseAddress);
			lpBuffer->Protect = PAGE_NOACCESS;
			lpBuffer->State = MEM_FREE;
			lpBuffer->Type = 0;
			return dwLength;
		}
	}
	return 0;
}



bool ReadProcessMemoryCR3(uint64_t cr3, void* lpBaseAddress, void* lpBuffer, uint32_t nSize, uintptr_t* lpNumberOfBytesRead) {
	uint64_t currentAddress;
	uint64_t PA;
	int i;
	uintptr_t* x;
	int blocksize;

	if (!verifyAddress(reinterpret_cast<uint64_t>(lpBaseAddress))) {
		lpNumberOfBytesRead = 0;
		return false;
	}

	cr3 = cr3 & MAXPHYADDRMASKPB;

	bool result = false;
	if (nSize == 0) return false;

	lpNumberOfBytesRead = 0;
	currentAddress = reinterpret_cast<uint64_t>(lpBaseAddress);

	// aligned memory from here on
	while (nSize > 0) {
		blocksize = std::min(nSize, 0x1000 - (currentAddress & 0xfff));

		if (!VirtualToPhysicalCR3(cr3, currentAddress, PA)) return false;
		if (!ReadPhysicalMemory(0, reinterpret_cast<void*>(PA), lpBuffer, blocksize, x)) return false;

		lpNumberOfBytesRead += reinterpret_cast<uintptr_t>(x);
		currentAddress += reinterpret_cast<uintptr_t>(x);
		lpBuffer = reinterpret_cast<void*>(reinterpret_cast<uint64_t>(lpBuffer) + reinterpret_cast<uint64_t>(x));
		nSize -= reinterpret_cast<uintptr_t>(x);
	}

	result = true;
	return result;
}

bool WriteProcessMemoryCR3(uint64_t cr3, void* lpBaseAddress, void* lpBuffer, uint32_t nSize, uintptr_t& lpNumberOfBytesWritten) {
	uint64_t currentAddress;
	uint64_t PA;
	int i;
	uintptr_t x;
	int blocksize;

	if (!verifyAddress(reinterpret_cast<uint64_t>(lpBaseAddress))) {
		lpNumberOfBytesWritten = 0;
		return false;
	}

	cr3 = cr3 & MAXPHYADDRMASKPB;

	bool result = false;
	if (nSize == 0) return false;

	lpNumberOfBytesWritten = 0;
	currentAddress = reinterpret_cast<uint64_t>(lpBaseAddress);

	// aligned memory from here on
	while (nSize > 0) {
		blocksize = std::min(nSize, 0x1000 - (currentAddress & 0xfff));


		// Replace the problematic line with the following
		if (!VirtualToPhysicalCR3(cr3, currentAddress, PA)) return false;
		if (!WritePhysicalMemory(0, reinterpret_cast<void*>(PA), lpBuffer, blocksize, x)) return false;

		lpNumberOfBytesWritten += x;
		currentAddress += x;
		lpBuffer = reinterpret_cast<void*>(reinterpret_cast<uint64_t>(lpBuffer) + x);
		nSize -= x;
	}

	result = true;
	return result;
}


BOOL WriteProcessMemory(HANDLE hProcess, LPCVOID lpBaseAddress, LPVOID lpBuffer, SIZE_T nSize, uint64_t* lpNumberOfBytesWritten) {
	TWriteLogEntry* wle;
	uintptr_t x;
	uint64_t cr3;

	if (!verifyAddress(reinterpret_cast<uint64_t>(lpBaseAddress))) {
		lpNumberOfBytesWritten = NULL;
		return FALSE;
	}

	BOOL result = FALSE;

	if (logWrites) {
		if (nSize < 64 * 1024 * 1024) {
			wle->address = reinterpret_cast<uintptr_t>(lpBaseAddress);
			wle->originalbytes = new BYTE[nSize];
			ReadProcessMemory(hProcess, lpBaseAddress, wle->originalbytes, nSize, &x);
			wle->originalsize = x;
		}
	}

	if (((reinterpret_cast<uint64_t>(lpBaseAddress) & (uint64_t(1) << 63)) != 0) && // kernelmode access
		(WriteProcessMemoryActual == defaultWPM) &&
		isRunningDBVM || DBVMWatchBPActive) {
		if (dbk32functions.GetCR3(hProcess, cr3)) { // todo: maybe just a getkernelcr3
			result = WriteProcessMemoryCR3(cr3, const_cast<void*>(lpBaseAddress), lpBuffer, nSize, *lpNumberOfBytesWritten);
		}
	}
	else {
		result = WriteProcessMemoryActual(hProcess, const_cast<void*>(lpBaseAddress), lpBuffer, nSize, lpNumberOfBytesWritten);
	}

	if (result && logWrites && wle != nullptr) {
		wle->newbytes = new BYTE[*lpNumberOfBytesWritten];
		ReadProcessMemory(hProcess, lpBaseAddress, wle->newbytes, *lpNumberOfBytesWritten, &x);
		wle->newsize = x;
		addWriteLogEntryToList(wle);
	}

	return result;
}


BOOL ReadProcessMemory(HANDLE hProcess, LPCVOID lpBaseAddress, LPVOID lpBuffer, SIZE_T nSize, uint64_t* lpNumberOfBytesWritten) {

	uint64_t cr3;
	BOOL result = FALSE;
	if (!verifyAddress(reinterpret_cast<uint64_t>(lpBaseAddress))) {
		lpNumberOfBytesWritten = NULL;
		return FALSE;
	}

	if (((reinterpret_cast<uint64_t>(lpBaseAddress) & (uint64_t(1) << 63)) != 0) && // kernelmode access
		(WriteProcessMemoryActual == defaultWPM) &&
		isRunningDBVM || DBVMWatchBPActive) {
		if (dbk32functions.GetCR3(hProcess, cr3)) { // todo: maybe just a getkernelcr3
			result = ReadProcessMemoryCR3(cr3, const_cast<void*>(lpBaseAddress), lpBuffer, nSize, lpNumberOfBytesWritten);
			return result;
		}
	}

	result = ReadProcessMemoryActual(hProcess, lpBaseAddress, lpBuffer, nSize, lpNumberOfBytesWritten);
	return result;
}


DWORD VirtualQueryEx(HANDLE hProcess, LPCVOID lpAddress, TMemoryBasicInformation* lpBuffer, DWORD dwLength) {
	uint64_t cr3;
	if (forceCR3VirtualQueryEx) {
		if (dbk32functions.GetCR3(hProcess, cr3)) {
			return VirtualQueryExCR3(cr3, (void*)lpAddress, lpBuffer, dwLength);
		}
	}
	return VirtualQueryExActual(hProcess, lpAddress, lpBuffer, dwLength);
}



BOOLEAN Is64BitProcess(HANDLE processhandle)
{
	BOOL iswow64 = FALSE;

	if (Is64bitOS()) {
		if (IsWow64Process(processhandle, &iswow64)) {
			if (iswow64) return FALSE;
			return TRUE;
		}
	}
	return FALSE;


}

BOOL NewKernelHandler::IsRunningDBVM()
{
	return dbvm_version() > 0;
}

BOOL NewKernelHandler::isIntel()
{
	int r[4];
	__cpuid(r, 0);
	if (r[1] == 0x756e6547 && r[2] == 0x69746e65 && r[3] == 0x444d4163) return TRUE; //GenuineIntel
	return FALSE;
}

BOOL NewKernelHandler::Is64bitOS()
{
#ifdef _WIN64
	return TRUE;
#else
	return FALSE;
#endif
}



void NeedsDBVM(const std::string& reason) {
	std::string r = reason;

	if (!IsRunningDBVM()) {
		if (r.empty()) {
			r = rsToUseThisFunctionYouWillNeedToRunDBVM;
		}
		if (isDBVMCapable && (MessageBoxW(0, std::wstring(r.begin(), r.end()).c_str(), std::wstring(r.begin(), r.end()).c_str(), MB_ICONWARNING | MB_YESNO) == IDYES)) {
			LaunchDBVM(-1);
			if (!IsRunningDBVM()) {
				throw std::exception(rsDidNotLoadDBVM);
			}

			if (!IsRunningDBVM()) {
				throw std::exception(rsDBVMIsNotLoadedThisFeatureIsNotUsable);
			}
		}

	}
}

BOOL  loaddbvmifneeded(const std::string& reason = "") {
#ifdef _WIN64

	if (IsRunningDBVM()) {
		return TRUE;
	}

	std::string r = reason.empty() ? rsToUseThisFunctionYouWillNeedToRunDBVM : reason;

	LoadDBK32();
	if (auto isDriverLoadedPtr = reinterpret_cast<decltype(isDriverLoaded)>(GetProcAddress(/*hdll*/nullptr, "isDriverLoaded"))) {
		BOOL signedStatus = FALSE;
		if (!isDriverLoadedPtr(&signedStatus)) {
			return FALSE;
		}

		if (Is64bitOS() && !IsRunningDBVM()) {
			if (!isDBVMCapable()) {
				throw std::exception(rsYourCpuMustBeAbleToRunDbvmToUseThisFunction);
			}

			if (signedStatus) {
				// 主线程检查及弹窗（需要GUI实现）
				if (GetCurrentThreadId() == MainThreadID &&
					MessageBoxA(nullptr, r.c_str(), "警告", MB_YESNO) != IDYES) {
					return FALSE;
				}

				LaunchDBVM(-1);
				if (!IsRunningDBVM()) {
					throw std::exception(rsDidNotLoadDBVM);
				}

				// 主线程UI更新（假设有MemoryBrowser对象）
				if (GetCurrentThreadId() == MainThreadID) {
					MemoryBrowser.Kerneltools1.enabled = true;
				}
				return TRUE;
			}
			else {
				throw std::exception(signedStatus ?
					rsPleaseRebootAndPressF8BeforeWindowsBoots :
					rsTheDriverNeedsToBeLoadedToBeAbleToUseThisFunction);
			}
		}
		else
		{
			return TRUE;
		}
	}
	return FALSE;

#endif
}




BOOL IsDBVMCapable()
{
	int r[4];
	__cpuid(r, 1);
	if (((r[2] >> 5) & 0x1) == 1) return TRUE;
}




bool hasEPTSupport() {
	const auto IA32_VMX_BASIC_MSR = 0x480;
	const auto IA32_VMX_TRUE_PROCBASED_CTLS_MSR = 0x48e;
	const auto IA32_VMX_PROCBASED_CTLS_MSR = 0x482;
	const auto IA32_VMX_PROCBASED_CTLS2_MSR = 0x48b;

	bool result = isDBVMCapable(); // Assume true until proven otherwise 
	DWORD procbased1flags;

	if (isDriverLoaded(nullptr)) {
		if (isIntel()) {
			result = false;
			ULONGLONG basic_msr = __readmsr(IA32_VMX_BASIC_MSR);

			// Check bit 55 to determine which control MSR to use 
			if ((basic_msr & (1ULL << 55)) != 0) {
				procbased1flags = static_cast<DWORD>(__readmsr(IA32_VMX_TRUE_PROCBASED_CTLS_MSR) >> 32);
			}
			else {
				procbased1flags = static_cast<DWORD>(__readmsr(IA32_VMX_PROCBASED_CTLS_MSR) >> 32);
			}

			// Check secondary processor-based controls 
			if ((procbased1flags & (1 << 31)) != 0) {
				ULONGLONG ctrls2 = __readmsr(IA32_VMX_PROCBASED_CTLS2_MSR) >> 32;
				if ((ctrls2 & (1ULL << 1)) != 0) {
					result = true;
				}
			}
		}
		else {
			result = false;
		}
	}

	return result;

}


void NewKernelHandler::LoadDBK32()
{
	if (!DBKLoaded)
	{
		OutputDebugStringA("LoadDBK32");

		DBK32Initialize();
		DBKLoaded = (dbk32functions->hdevice != 0) && (dbk32functions->hdevice != INVALID_HANDLE_VALUE);



		// 函数指针赋值 
		GetThreadsProcessOffset = dbk32functions->GetThreadsProcessOffset;
		GetThreadListEntryOffset = dbk32functions->GetThreadListEntryOffset;
		GetDebugportOffset = dbk32functions->GetDebugportOffset;

		GetCR4 = dbk32functions->GetCR4;
		GetCR3 = dbk32functions->GetCR3;
		GetCR0 = dbk32functions->GetCR0;
		GetSDT = dbk32functions->GetSDT;
		GetSDTShadow = dbk32functions->GetSDTShadow;

		StartProcessWatch = dbk32functions->StartProcessWatch;
		WaitForProcessListData = dbk32functions->WaitForProcessListData;
		GetProcessNameFromID = dbk32functions->GetProcessNameFromID;
		GetProcessNameFromPEProcess = dbk32functions->GetProcessNameFromPEProcess;

		GetIDTs = dbk32functions->GetIDTs;
		GetIDTCurrentThread = dbk32functions->GetIDTCurrentThread;
		GetGDT = dbk32functions->GetGDT;
		MakeWritable = dbk32functions->MakeWritable;
		GetLoadedState = dbk32functions->GetLoadedState;

		DBKResumeThread = dbk32functions->DBKResumeThread;
		DBKSuspendThread = dbk32functions->DBKSuspendThread;

		DBKResumeProcess = dbk32functions->DBKResumeProcess;
		DBKSuspendProcess = dbk32functions->DBKSuspendProcess;

		KernelAlloc = dbk32functions->KernelAlloc;
		KernelAlloc64 = dbk32functions->KernelAlloc64;
		GetKProcAddress = dbk32functions->GetKProcAddress;
		GetKProcAddress64 = dbk32functions->GetKProcAddress64;

		GetSDTEntry = dbk32functions->GetSDTEntry;
		GetSSDTEntry = dbk32functions->GetSSDTEntry;

		LaunchDBVM = dbk32functions->LaunchDBVM;

		CreateRemoteAPC = dbk32functions->CreateRemoteAPC;


		//Debug message
		DBKDebug_ContinueDebugEvent = debug::DBKDebug_ContinueDebugEvent;
		DBKDebug_WaitForDebugEvent = debug.DBKDebug_WaitForDebugEvent;
		DBKDebug_GetDebuggerState = debug.DBKDebug_GetDebuggerState;
		DBKDebug_SetDebuggerState = debug.DBKDebug_SetDebuggerState;
		DBKDebug_SetGlobalDebugState = debug.DBKDebug_SetGlobalDebugState;
		DBKDebug_SetAbilityToStepKernelCode = debug.DBKDebug_SetAbilityToStepKernelCode;
		DBKDebug_StartDebugging = debug.DBKDebug_StartDebugging;
		DBKDebug_StopDebugging = debug.DBKDebug_StopDebugging;
		DBKDebug_GD_SetBreakpoint = debug.DBKDebug_GD_SetBreakpoint;



		if (pluginhandler != nullptr)  pluginhandler.handlechangedpointers(0);

		MemoryBrowser.Kerneltools1.Enabled = DBKLoaded | IsRunningDBVM();
		MemoryBrowser.miCR3Switcher.visible = MemoryBrowser.Kerneltools1.Enabled;
	}
}



void NewKernelHandler::DBKFileAsMemory(const std::string& fn, uintptr_t* baseaddress = 0)
{
	filehandler->filename = fn;

	if (filehandler->filedata != nullptr) {
		delete filehandler->filedata;
		filehandler->filedata = nullptr;
	}

	filehandler->filedata = new fstream();
	filehandler->filedata->open(fn, ios::binary | ios::in | ios::out);
	filehandler->filebaseaddress = baseaddress;

	// 假设存在无参数重载版本 
	DBKFileAsMemory();
}

void NewKernelHandler::DBKFileAsMemory()
{
	UseFileAsMemory = true;
	Usephysical = false;
	Usephysicaldbvm = false;
	ReadProcessMemoryActual = ReadProcessMemoryFile;
	WriteProcessMemoryActual = WriteProcessMemoryFile;
	VirtualQueryExActual = VirtualQueryExFile;

	if (pluginhandler != nullptr) pluginhandler->handlechangedpointers(3);
}
