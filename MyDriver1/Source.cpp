#include "Source.h"
#include "SSDT.h"
#include "Utils.h"

#define PROCESS_TO_PROTECT "calc.exe"

const UCHAR ProhibitedAccessRights[] = {
	//PROCESS_TERMINATE,
	PROCESS_VM_READ,
	PROCESS_VM_OPERATION,
	PROCESS_VM_WRITE
};

extern "C" DRIVER_INITIALIZE DriverEntry;

typedef NTSTATUS(*ZwTerminateProcess_t)(HANDLE, ULONG);

extern "C" VOID OnUnload(struct _DRIVER_OBJECT* DriverObject) {

	UNREFERENCED_PARAMETER(DriverObject);

	DbgPrint("Unloading self ...\n");
}

extern "C" void PostOperationCallback(
	PVOID RegistrationContext,
	POB_POST_OPERATION_INFORMATION OperationInformation
) {

	UNREFERENCED_PARAMETER(RegistrationContext);
	UNREFERENCED_PARAMETER(OperationInformation);
}

VOID Log(PCSTR format, ...) {
	va_list args;
	va_start(args, format);

	vDbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, format, args);

	va_end(args);
}

extern "C" OB_PREOP_CALLBACK_STATUS PreOperationCallback(
	PVOID RegistrationContext,
	POB_PRE_OPERATION_INFORMATION OperationInformation
) {

	UNREFERENCED_PARAMETER(RegistrationContext);

	// getting process, that was touched
	PEPROCESS Target = (PEPROCESS)OperationInformation->Object;

	// reading process name from PEPROCESS 

	UCHAR imageName[15];

	GetImageNameFromProcess(Target, imageName);

	// if process name is calc.exe ...
	if (!strcmp((const char*)imageName, PROCESS_TO_PROTECT)) {

		// taking access flags, which were used in handle creation, and if it contains prohibited right - we will remove it
		
		for(int i = 0; i < sizeof(ProhibitedAccessRights); i++) {

			UCHAR prohibitedAccess = ProhibitedAccessRights[i];

			if (OperationInformation->Parameters->CreateHandleInformation.OriginalDesiredAccess & prohibitedAccess) {
				OperationInformation->Parameters->CreateHandleInformation.DesiredAccess &= ~prohibitedAccess;
			}
		}
		
		
	}



	return OB_PREOP_SUCCESS;
}

NTSTATUS(*Real_ZwTerminateProcess)(HANDLE Handle, ULONG ExitCode);

NTSTATUS MyTerminate(HANDLE Handle, ULONG ExitCode)
{

	Log("My terminate called for handle %p!\n", Handle);

	//if(Handle <= 0) {
	//	return STATUS_INVALID_HANDLE;
	//}

	PEPROCESS process;

	NTSTATUS status = ObReferenceObjectByHandle(Handle, FILE_READ_DATA, NULL, KernelMode, (PVOID*)&process, NULL);

	if(!NT_SUCCESS(status)) {
		Log("[!] Error calling ObReferenceObjectByHandle(): %p\n", status);
		return STATUS_SUCCESS;
	}

	UCHAR processName[15];

	GetImageNameFromProcess(process, processName);

	if(!strcmp((const char*)processName, PROCESS_TO_PROTECT)) {

		PEPROCESS _tmp = PsGetCurrentProcess();
		UCHAR tmpImagename[15];

		GetImageNameFromProcess(_tmp, tmpImagename);

		if(!strcmp((const char*)tmpImagename, PROCESS_TO_PROTECT)) {
			return Real_ZwTerminateProcess(Handle, ExitCode);
		}

		Log("[*] You tried to kill %s! DIE!\nPerson, who tried to kill: %s\n\n", PROCESS_TO_PROTECT, tmpImagename);

		return STATUS_ACCESS_DENIED;
	}

	return Real_ZwTerminateProcess(Handle, ExitCode);
}



extern "C" NTSTATUS DriverEntry(
	struct _DRIVER_OBJECT* DriverObject,
	PUNICODE_STRING  RegistryPath
)
{

	UNREFERENCED_PARAMETER(DriverObject);
	UNREFERENCED_PARAMETER(RegistryPath);
	
	// setting callback that will be executed on driver unloading
	DriverObject->DriverUnload = OnUnload;

	InitSSDTHooker();
	Real_ZwTerminateProcess = (ZwTerminateProcess_t)HookSSDT((DWORD32)ZwTerminateProcess, (DWORD32)MyTerminate);

	HANDLE regHandle;

	// registering callbacks
	NTSTATUS status = InitObCallbacks(PreOperationCallback, PostOperationCallback, &regHandle);

	if (!NT_SUCCESS(status)) {
		Log("Error: %p\n", status);
	} else {
		Log("Well done!\n");
	}

	Log("NEP NEP\n");
	return STATUS_SUCCESS;
}