#include "Source.h"
#include "SSDT.h"
#include "Utils.h"

#define PROCESS_TO_PROTECT "calc.exe"

extern "C" DRIVER_INITIALIZE DriverEntry;

NTSTATUS(*Real_ZwTerminateProcess)(HANDLE Handle, ULONG ExitCode);

void OnImageLoaded(
	PUNICODE_STRING FullImageName,
	HANDLE ProcessId,
	PIMAGE_INFO ImageInfo
) {

	UNREFERENCED_PARAMETER(FullImageName);

	PEPROCESS process;
	NTSTATUS result = PsLookupProcessByProcessId((HANDLE)ProcessId, &process);

	if (!NT_SUCCESS(result)) {
		Log("[!] Error getting process by PID %i!\n", ProcessId);
		return;
	}

	PEPROCESS caller = PsGetCurrentProcess();

	UCHAR name[15];
	GetImageNameFromProcess(caller, name);

	ANSI_STRING str;

	RtlUnicodeStringToAnsiString(&str, FullImageName, TRUE);

	Log("[*] Image %s loaded with base %p into %s!\n", str.Buffer, ImageInfo->ImageBase, name);

	RtlFreeAnsiString(&str);
}

NTSTATUS MyTerminate(HANDLE Handle, ULONG ExitCode)
{

	Log("My terminate called for handle %p!\n", Handle);

	PEPROCESS process;

	NTSTATUS status = ObReferenceObjectByHandle(Handle, FILE_READ_DATA, NULL, KernelMode, (PVOID*)& process, NULL);

	if (!NT_SUCCESS(status)) {
		Log("[!] Error calling ObReferenceObjectByHandle(): %p\n", status);
		return STATUS_SUCCESS;
	}

	UCHAR processName[15];

	GetImageNameFromProcess(process, processName);

	if (!strcmp((const char*)processName, PROCESS_TO_PROTECT)) {

		PEPROCESS _tmp = PsGetCurrentProcess();
		UCHAR tmpImagename[15];

		GetImageNameFromProcess(_tmp, tmpImagename);

		if (!strcmp((const char*)tmpImagename, PROCESS_TO_PROTECT)) {
			return Real_ZwTerminateProcess(Handle, ExitCode);
		}

		Log("[*] You tried to kill %s! DIE!\nPerson, who tried to kill: %s\n\n", PROCESS_TO_PROTECT, tmpImagename);

		return STATUS_ACCESS_DENIED;
	}

	return Real_ZwTerminateProcess(Handle, ExitCode);
}

SSDT_HookEntry HookedApis[TOTAL_HOOKS];

VOID InitHookTable() {

	SSDT_HookEntry zwTerminateProcess;
	zwTerminateProcess.FunctionAddress = (DWORD32)ZwTerminateProcess;
	zwTerminateProcess.HookAddress = (DWORD32)MyTerminate;
	zwTerminateProcess.OriginalFunctionAddress = (PDWORD32)&Real_ZwTerminateProcess;

	HookedApis[0] = zwTerminateProcess;
}

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

				PEPROCESS _tmp = PsGetCurrentProcess();

				UCHAR tmpImagename[15];

				GetImageNameFromProcess(_tmp, tmpImagename);

				Log("[*] %s tried to violate the law with action %p!\n", tmpImagename, prohibitedAccess);

				OperationInformation->Parameters->CreateHandleInformation.DesiredAccess &= ~prohibitedAccess;
			}
		}
		
		
	}

	return OB_PREOP_SUCCESS;
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
	InitHookTable();

	//PsSetCreateProcessNotifyRoutine(OnProcessCreated, FALSE);
	PsSetLoadImageNotifyRoutine(OnImageLoaded);

	// making hook for each HookedApi entry
	for (int i = 0; i < TOTAL_HOOKS; i++) {

		SSDT_HookEntry entry = HookedApis[i];

		Log("[*] Hooking %p to the %p\n", entry.FunctionAddress, entry.HookAddress);

		*entry.OriginalFunctionAddress = HookSSDT((DWORD32)entry.FunctionAddress, (DWORD32)entry.HookAddress);
	}

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