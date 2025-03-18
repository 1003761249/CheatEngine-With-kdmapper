#pragma once
#include <ntifs.h>
#include <ntimage.h>
#include "nt.h"
#include "utils.h"
#include "IOPLDispatcher.h"
#include "DBKDrvr.h"



PDEVICE_OBJECT GetDeviceObjectByName(PWCH DriverName)
{
	UNICODE_STRING uName = { 0 };

	RtlInitUnicodeString(&uName, DriverName);

	PFILE_OBJECT FileObj = NULL;

	PDEVICE_OBJECT pDevice = NULL;

	NTSTATUS status = IoGetDeviceObjectPointer(&uName, FILE_ALL_ACCESS, &FileObj, &pDevice);

	if (!NT_SUCCESS(status))return NULL;

	if (FileObj)ObDereferenceObject(FileObj);

	return pDevice;
}

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
)
{
	//__debugbreak();
	if (InputBuffer != NULL && MmIsAddressValid(InputBuffer) && MmIsAddressValid((PUCHAR)InputBuffer + InputBufferLength - 1))
	{
		IRP FakeIRP;
		PCommInfo buffer = { 0 };
		buffer = ExAllocatePool(NonPagedPool, max(InputBufferLength, OutputBufferLength));
		buffer->inputBuffer = ExAllocatePool(NonPagedPool, max(InputBufferLength, OutputBufferLength));



		//__debugbreak();
		//__debugbreak();

		//尝试第二种思路获取三环数据
		//CommInfo R3_data = {0};
		//memcpy(&(R3_data.ControlCode), InputBuffer, sizeof(UINT32));
		//R3_data.inputBuffer = 0;
		//memcpy(&(R3_data.inputBuffer), ((PULONG64)InputBuffer)[1],sizeof(uintptr_t));//此时R3_data.inputBuffer传入的是三环的正常io请求的inputbuff地址



		
		//PCommInfo Ring3_origin_input = InputBuffer;// 这三行是不能删除的
		//PVOID Normal_DeviceIobuff = Ring3_origin_input->inputBuffer;//正确的
		//PWCHAR  ring3path = ((pinputpath)Normal_DeviceIobuff)->dbvmpath;//测试读地址正确的


		//这四行尝试把第一种思路实现
		PCommInfo Ring3_origin_input = InputBuffer;
		PVOID Normal_DeviceIobuff = ExAllocatePool(NonPagedPool, 0x1000);
		memcpy(Normal_DeviceIobuff, Ring3_origin_input->inputBuffer, InputBufferLength);
		FakeIRP.AssociatedIrp.SystemBuffer = Normal_DeviceIobuff;
		//Normal_DeviceIobuff //申请内存
		//	把Ring3->inputbuffer拷贝到里面//
        //然后把Normal_device 作为irp的缓冲区 

		
		memcpy(buffer, InputBuffer, sizeof(ULONG32));
		memcpy(buffer->inputBuffer, InputBuffer, InputBufferLength);
		

		//FakeIRP.AssociatedIrp.SystemBuffer = buffer->inputBuffer;
		FakeIRP.Flags = buffer->ControlCode; //(ab)using an unused element 至少一个可以正确获取控制码的方法
		
		DbgPrintEx(0, 0, "MyDeviceIoControl %I64x\n", buffer->ControlCode);

		//__debugbreak();
		if (buffer->ControlCode != NULL)
		{
			DispatchIoctl(DeviceObject, &FakeIRP);
		}
		/*if (buffer->ControlCode == 0x1234567)
		{
			UnloadDriver(NULL);
		}*/

		memcpy(OutputBuffer, Normal_DeviceIobuff, OutputBufferLength);
		ExFreePool(Normal_DeviceIobuff);
		ExFreePool(buffer->inputBuffer);
		ExFreePool(buffer);

	}
	return TRUE;
}

NTSTATUS Hook_Setting_Device_FastIoDeviceControl()
{
	PDEVICE_OBJECT pDevice = GetDeviceObjectByName(HOOK_DEVICE_NAME);
	if (!pDevice)return FALSE;

	PDRIVER_OBJECT pDriver = pDevice->DriverObject;
	pDriver->FastIoDispatch->FastIoDeviceControl = MyDeviceIoControl;

	return STATUS_SUCCESS;
}

PVOID GetNtConvertBetweenAuxiliaryCounterAndPerformanceCounter_Addr()
{
	PVOID addr = GetKernelModuleAddress("ntoskrnl.exe");
	ULONG section_Size = { 0 };
	if (addr != NULL)
	addr = FindSection("PAGE", addr, &section_Size);
	//__debugbreak();
	if(addr !=NULL)
		addr = FindPattern(addr, section_Size, (BYTE*)"\x48\x8b\xc4\x48\x89\x58\x08\x48\x89\x70\x10\x48\x89\x78\x18\x41\x56\x48\x83\xec\x40\x49\x8b\xd9\x49\x8b\xf8\x40\x8a\xf1\x48\x83\x60\xe0\x00\x48\x83\x60\xd8\x00\x65\x48\x8b\x04\x25\x88\x01\x00", "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
	//__debugbreak();

	
	return addr;
}

//NtConvertBetweenAuxiliaryCounterAndPerformanceCounter
//48 8b c4 48 89 58 08 48 89 70 10 48 89 78 18 41
//56 48 83 ec 40 49 8b d9 49 8b f8 40 8a f1 48 83
//60 e0 00 48 83 60 d8 00 65 48 8b 04 25 88 01 00

uintptr_t GetKernalMutableFunctionAddr()
{
	//__debugbreak();
	uintptr_t return_addr = GetNtConvertBetweenAuxiliaryCounterAndPerformanceCounter_Addr();
	
	if (return_addr != NULL)
	{
		//__debugbreak();
		return_addr = return_addr + 0xB4 + 0x3;
		UINT32 offset = ((BYTE*)return_addr)[3] << 24|((BYTE*)return_addr)[2] << 16 | ((BYTE*)return_addr)[1] << 8 | ((BYTE*)return_addr)[0];
		return_addr = return_addr + 0x4 + offset;
	}
	//*(BYTE*)return_addr = 0xCC; //尝试在ntoskrnl的.data可写段添加一个断点，用于测试
	return return_addr;
}

BOOL Hook_CE_DeviceIoControl_With_noExportKenal()
{
	uintptr_t Change_addr = GetKernalMutableFunctionAddr();
	if (Change_addr != NULL)
	*(PULONG64)Change_addr = MyDeviceIoControl;
	else
	{
		DbgPrintEx(0, 0, "fail find Mutable addr");
	}
	return TRUE;
}


uintptr_t GetKernelModuleAddress(const char* module_name) {
	void* buffer = 0;
	ULONG32 buffer_size = 0;

	NTSTATUS status = ZwQuerySystemInformation(SystemModuleInformation, buffer, buffer_size, &buffer_size);

	while (status == STATUS_INFO_LENGTH_MISMATCH) {
		if (buffer != NULL)
			ExFreePool(buffer, 0, MEM_RELEASE);

		buffer = ExAllocatePool(NonPagedPool, buffer_size);
		status = ZwQuerySystemInformation(SystemModuleInformation, buffer, buffer_size, &buffer_size);
	}

	if (!NT_SUCCESS(status)) {
		if (buffer != NULL)
			ExFreePool(buffer);
		return 0;
	}

	PRTL_PROCESS_MODULES modules = (PRTL_PROCESS_MODULES)buffer;
	if (!modules)
		return 0;
	for (auto i = 0u; i < modules->NumberOfModules; ++i) {
		const char* current_module_name = (char*)(modules->Modules[i].FullPathName) + modules->Modules[i].OffsetToFileName;

		if (!_stricmp(current_module_name, module_name))
		{
			//__debugbreak();
			uintptr_t result = modules->Modules[i].ImageBase;

			ExFreePool(buffer, 0, MEM_RELEASE);
			return result;
		}
	}

	ExFreePool(buffer, 0, MEM_RELEASE);
	return 0;
}

BOOLEAN bDataCompare(const BYTE* pData, const BYTE* bMask, const char* szMask) {
	for (; *szMask; ++szMask, ++pData, ++bMask)
		if (*szMask == 'x' && *pData != *bMask)
			return 0;
	return (*szMask) == 0;
}

//这里的dwLen要的是一个实际的值而不是存长度的地址，定义uintptr_t为了方便数据转换不出错
uintptr_t FindPattern(uintptr_t dwAddress, uintptr_t dwLen, BYTE* bMask, const char* szMask) {
	size_t max_len = dwLen - strlen(szMask);
	for (uintptr_t i = 0; i < max_len; i++)
		if (bDataCompare((BYTE*)(dwAddress + i), bMask, szMask))
			return (uintptr_t)(dwAddress + i);
	return 0;
}

//这里的size需要的是存储size的地址
uintptr_t FindSection(const char* sectionName, uintptr_t modulePtr, PULONG size) {
	size_t namelength = strlen(sectionName);
	PIMAGE_NT_HEADERS headers = (PIMAGE_NT_HEADERS)(modulePtr + ((PIMAGE_DOS_HEADER)modulePtr)->e_lfanew);
	PIMAGE_SECTION_HEADER sections = IMAGE_FIRST_SECTION(headers);
	for (DWORD i = 0; i < headers->FileHeader.NumberOfSections; ++i) {
		PIMAGE_SECTION_HEADER section = &sections[i];
		//__debugbreak();
		if (memcmp(section->Name, sectionName, namelength) == 0 &&
			namelength == strlen((char*)section->Name)) {

			if (!section->VirtualAddress) {
				return 0;
			}
			if (size) {
				*size = section->Misc.VirtualSize;
			}
			//__debugbreak();
			return (uintptr_t)(modulePtr + section->VirtualAddress);
		}
	}
	return 0;
}