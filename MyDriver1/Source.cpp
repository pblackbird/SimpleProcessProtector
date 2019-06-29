#include "Source.h"

#define PROCESS_TO_PROTECT "calc.exe"

extern "C" DRIVER_INITIALIZE DriverEntry;

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

	PEPROCESS Target = (PEPROCESS)OperationInformation->Object;
	DWORD32 ImageFilename = (DWORD32)Target + 0x16c;

	UCHAR imageName[15];

	RtlCopyMemory(imageName, (const void*)ImageFilename, 15);

	if (!strcmp((const char*)imageName, PROCESS_TO_PROTECT)) {
		if(OperationInformation->Parameters->CreateHandleInformation.OriginalDesiredAccess & PROCESS_TERMINATE) {
			OperationInformation->Parameters->CreateHandleInformation.DesiredAccess &= ~PROCESS_TERMINATE;
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
	
	DriverObject->DriverUnload = OnUnload;

	OB_CALLBACK_REGISTRATION Registrator;
	OB_OPERATION_REGISTRATION Operation;
	REG_CONTEXT RegistrationContext;

	Operation.ObjectType = PsProcessType;
	Operation.Operations = OB_OPERATION_HANDLE_CREATE;
	Operation.PostOperation = PostOperationCallback;
	Operation.PreOperation = PreOperationCallback;

	Registrator.Version = OB_FLT_REGISTRATION_VERSION;
	Registrator.OperationRegistrationCount = 1;
	RtlInitUnicodeString(&Registrator.Altitude, L"XXXXXXX");
	Registrator.OperationRegistration = &Operation;
	Registrator.RegistrationContext = &RegistrationContext;

	HANDLE RegHandle;

	NTSTATUS status = ObRegisterCallbacks(&Registrator, &RegHandle);

	if (!NT_SUCCESS(status)) {
		Log("Error: %p\n", status);
	} else {
		Log("Well done!\n");
	}

	Log("NEP NEP\n");
	return STATUS_SUCCESS;
}