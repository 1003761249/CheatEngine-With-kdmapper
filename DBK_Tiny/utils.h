#pragma once
#include <minwindef.h>
#include <ntifs.h>
#include "nt.h"

#define HOOK_DEVICE_NAME L"\\Device\\Null"

typedef struct  __CommInfo
{
	UINT32 ControlCode;
	PVOID inputBuffer;
}CommInfo, * PCommInfo;

NTSTATUS NTAPI ZwQuerySystemInformation(
	_In_      SYSTEM_INFORMATION_CLASS SystemInformationClass,
	_Inout_   PVOID                    SystemInformation,
	_In_      ULONG                    SystemInformationLength,
	_Out_opt_ PULONG                   ReturnLength
);

BOOLEAN MyDeviceIoControl(
	_In_ struct _FILE_OBJECT* FileObject,
	_In_ BOOLEAN Wait,
	_In_opt_ PVOID InputBuffer,
	_In_ ULONG InputBufferLength,
	_Out_opt_ PVOID OutputBuffer,
	_In_ ULONG OutputBufferLength,
	_In_ ULONG IoControlCode,
	_Out_ PIO_STATUS_BLOCK IoStatus,
	_In_ struct _DEVICE_OBJECT* DeviceObject
);
uintptr_t GetKernalMutableFunctionAddr();
NTSTATUS Hook_Setting_Device_FastIoDeviceControl();
BOOL Hook_CE_DeviceIoControl_With_noExportKenal();
uintptr_t GetKernelModuleAddress(const char* module_name);
BOOLEAN bDataCompare(const BYTE* pData, const BYTE* bMask, const char* szMask);
uintptr_t FindPattern(uintptr_t dwAddress, uintptr_t dwLen, BYTE* bMask, const char* szMask);
uintptr_t FindSection(const char* sectionName, uintptr_t modulePtr, PULONG size);
