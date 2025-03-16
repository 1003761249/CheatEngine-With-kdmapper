#pragma warning( disable: 4100 4101 4103 4189)

#include "DBKFunc.h"
#include <ntifs.h>
#include <windef.h>
#include "DBKDrvr.h"
#include "processlist.h"
#include "memscan.h"
#include "threads.h"
#include "vmxhelper.h"
#include "debugger.h"
#include "vmxoffload.h"
#include "IOPLDispatcher.h"
#include "interruptHook.h"
#include "ultimap.h"
#include "ultimap2.h"
#include "noexceptions.h"
#include "apic.h"
#include "../utils.h"


NTSTATUS DispatchCreate(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS DispatchClose(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);



typedef NTSTATUS(*PSRCTNR)(__in PCREATE_THREAD_NOTIFY_ROUTINE NotifyRoutine);
PSRCTNR PsRemoveCreateThreadNotifyRoutine2;

typedef NTSTATUS(*PSRLINR)(__in PLOAD_IMAGE_NOTIFY_ROUTINE NotifyRoutine);
PSRLINR PsRemoveLoadImageNotifyRoutine2;

UNICODE_STRING  uszDeviceString;
void* functionlist[1];
char  paramsizes[1];
int registered = 0;



NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegistryPath)
{
	NTSTATUS        ntStatus = STATUS_SUCCESS;
	UNICODE_STRING temp;
	criticalSection csTest;
	HANDLE Ultimap2Handle;
	KernelCodeStepping = 0;
	KernelWritesIgnoreWP = 0;

	manual_load = TRUE;
	//__debugbreak();
	if (RegistryPath == 0x1234)
		manual_load = TRUE;
	else if(RegistryPath)
		loadedbydbvm = TRUE;


	if (!loadedbydbvm && !manual_load)
	{
		DriverObject->DriverUnload = UnloadDriver;
		DriverObject->MajorFunction[IRP_MJ_CREATE] = DispatchCreate;
		DriverObject->MajorFunction[IRP_MJ_CLOSE] = DispatchClose;
		DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchIoctl;
	}

	//PVOID adrr = GetMutableAddr();
	if (loadedbydbvm)
		DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = (PDRIVER_DISPATCH)DispatchIoctlDBVM;
	 if (manual_load)
	{
		PVOID adrr = GetKernalMutableFunctionAddr();
		Hook_Setting_Device_FastIoDeviceControl();
	}

	if (DriverObject != NULL)
	{
		DriverObject->DriverUnload = UnloadDriver;
	}


	//Processlist init
	ProcessEventCount = 0;
	ExInitializeResourceLite(&ProcesslistR);


	CreateProcessNotifyRoutineEnabled = FALSE;

	//threadlist init
	ThreadEventCount = 0;

	processlist = NULL;

	PTESize = 8; //pae
	PAGE_SIZE_LARGE = 0x200000;
	//base was 0xfffff68000000000ULL

	//to 
	MAX_PTE_POS = 0xFFFFF6FFFFFFFFF8ULL; // base + 0x7FFFFFFFF8
	MAX_PDE_POS = 0xFFFFF6FB7FFFFFF8ULL; // base + 0x7B7FFFFFF8



	//hideme(DriverObject); //ok, for those that see this, enabling this WILL fuck up try except routines, even in usermode you'll get a blue sreen

	DbgPrint("Initializing debugger\n");
	debugger_initialize();

	// Return success (don't do the devicestring, I need it for unload)



	//fetch cpu info  ¼ì²écpuÐÅÏ¢
	{
		DWORD r[4];
		DWORD a;

		__cpuid(r, 0);
		DbgPrintEx(0,0,"check cpu art cpuid.0: r[1]=%x", r[1]);
		if (r[1] == 0x756e6547) //GenuineIntel
		{

			__cpuid(r, 1);

			a = r[0];

			cpu_stepping = a & 0xf;
			cpu_model = (a >> 4) & 0xf;
			cpu_familyID = (a >> 8) & 0xf;
			cpu_type = (a >> 12) & 0x3;
			cpu_ext_modelID = (a >> 16) & 0xf;
			cpu_ext_familyID = (a >> 20) & 0xff;

			cpu_model = cpu_model + (cpu_ext_modelID << 4);
			cpu_familyID = cpu_familyID + (cpu_ext_familyID << 4);

			vmx_init_dovmcall(1);
			setup_APIC_BASE(); //for ultimap

		}
		else
		{
			DbgPrint("Not an intel cpu");
			if (r[1] == 0x68747541)
			{
				DbgPrint("This is an AMD\n");
				vmx_init_dovmcall(0);
			}

		}



	}

#ifdef DEBUG1
	{
		APIC y;

		DebugStackState x;
		DbgPrint("offset of LBR_Count=%d\n", (UINT_PTR)&x.LBR_Count - (UINT_PTR)&x);


		DbgPrint("Testing forEachCpu(...)\n");
		forEachCpu(TestDPC, NULL, NULL, NULL, NULL);

		DbgPrint("Testing forEachCpuAsync(...)\n");
		forEachCpuAsync(TestDPC, NULL, NULL, NULL, NULL);

		DbgPrint("Testing forEachCpuPassive(...)\n");
		forEachCpuPassive(TestPassive, 0);

		DbgPrint("LVT_Performance_Monitor=%x\n", (UINT_PTR)&y.LVT_Performance_Monitor - (UINT_PTR)&y);
	}
#endif

#ifdef DEBUG2
	DbgPrint("No exceptions test:");
	if (NoExceptions_Enter())
	{
		int o = 45678;
		int x = 0, r = 0;
		//r=NoExceptions_CopyMemory(&x, &o, sizeof(x));

		r = NoExceptions_CopyMemory(&x, (PVOID)0, sizeof(x));

		DbgPrint("o=%d x=%d r=%d", o, x, r);


		DbgPrint("Leaving NoExceptions mode");
		NoExceptions_Leave();
	}
#endif


	RtlInitUnicodeString(&temp, L"PsSuspendProcess");
	PsSuspendProcess = (PSSUSPENDPROCESS)MmGetSystemRoutineAddress(&temp);

	RtlInitUnicodeString(&temp, L"PsResumeProcess");
	PsResumeProcess = (PSSUSPENDPROCESS)MmGetSystemRoutineAddress(&temp);

	return STATUS_SUCCESS;
}


NTSTATUS DispatchCreate(IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	// Check for SeDebugPrivilege. (So only processes with admin rights can use it)

	LUID sedebugprivUID;
	sedebugprivUID.LowPart = SE_DEBUG_PRIVILEGE;
	sedebugprivUID.HighPart = 0;

	Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;



	if (SeSinglePrivilegeCheck(sedebugprivUID, UserMode))
	{
		Irp->IoStatus.Status = STATUS_SUCCESS;
	}
	else
	{
		DbgPrint("A process without SeDebugPrivilege tried to open the dbk driver\n");
		Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
	}

	Irp->IoStatus.Information = 0;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return Irp->IoStatus.Status;
}


NTSTATUS DispatchClose(IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return Irp->IoStatus.Status;
}





void UnloadDriver(PDRIVER_OBJECT DriverObject)
{
	cleanupDBVM();

	if (!debugger_stopDebugging())
	{
		DbgPrint("Can not unload the driver because of debugger\n");
		return; //
	}

	debugger_shutdown();

	ultimap_disable();
	DisableUltimap2();
	UnregisterUltimapPMI();

	clean_APIC_BASE();

	NoExceptions_Cleanup();

	if (CreateProcessNotifyRoutineEnabled)
	{
		PVOID x;
		UNICODE_STRING temp;

		RtlInitUnicodeString(&temp, L"PsRemoveCreateThreadNotifyRoutine");
		PsRemoveCreateThreadNotifyRoutine2 = (PSRCTNR)MmGetSystemRoutineAddress(&temp);

		RtlInitUnicodeString(&temp, L"PsRemoveCreateThreadNotifyRoutine");
		PsRemoveLoadImageNotifyRoutine2 = (PSRLINR)MmGetSystemRoutineAddress(&temp);

		RtlInitUnicodeString(&temp, L"ObOpenObjectByName");
		x = MmGetSystemRoutineAddress(&temp);

		DbgPrint("ObOpenObjectByName=%p\n", x);


		if ((PsRemoveCreateThreadNotifyRoutine2) && (PsRemoveLoadImageNotifyRoutine2))
		{
			DbgPrint("Stopping processwatch\n");

			if (CreateProcessNotifyRoutineEnabled)
			{
				DbgPrint("Removing process watch");
#if (NTDDI_VERSION >= NTDDI_VISTASP1)
				PsSetCreateProcessNotifyRoutineEx(CreateProcessNotifyRoutineEx, TRUE);
#else
				PsSetCreateProcessNotifyRoutine(CreateProcessNotifyRoutine, TRUE);
#endif


				DbgPrint("Removing thread watch");
				PsRemoveCreateThreadNotifyRoutine2(CreateThreadNotifyRoutine);
			}

		}
		else return;  //leave now!!!!!		
	}


	DbgPrint("Driver unloading\n");


	CleanProcessList();

	ExDeleteResourceLite(&ProcesslistR);

	RtlZeroMemory(&ProcesslistR, sizeof(ProcesslistR));

#if (NTDDI_VERSION >= NTDDI_VISTA)
	if (DRMHandle)
	{
		DbgPrint("Unregistering DRM handle");
		ObUnRegisterCallbacks(DRMHandle);
		DRMHandle = NULL;
	}
#endif
}
