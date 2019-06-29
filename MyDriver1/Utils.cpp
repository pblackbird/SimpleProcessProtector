#include "Utils.h"
#include "Types.h"

VOID GetImageNameFromProcess(_In_ PEPROCESS Target, _Out_ UCHAR* output) {
	DWORD32 ImageFilename = (DWORD32)Target + 0x16c;

	RtlCopyMemory(output, (const void*)ImageFilename, 15);
}

NTSTATUS InitObCallbacks(
	_In_ POB_PRE_OPERATION_CALLBACK preOperation,
	_In_ POB_POST_OPERATION_CALLBACK postOperation,
	_Out_ PHANDLE handle
) {

	OB_CALLBACK_REGISTRATION Registrator;
	OB_OPERATION_REGISTRATION Operation;
	REG_CONTEXT RegistrationContext;

	// we want to create callbacks that will catch handle creations of processes
	Operation.ObjectType = PsProcessType;
	Operation.Operations = OB_OPERATION_HANDLE_CREATE;

	// placing callbacks, once that will be called BEFORE operation, and another will be called AFTER
	Operation.PostOperation = postOperation;
	Operation.PreOperation = preOperation;

	// filling stucture that holds info about our callbacks
	Registrator.Version = OB_FLT_REGISTRATION_VERSION;
	Registrator.OperationRegistrationCount = 1;
	RtlInitUnicodeString(&Registrator.Altitude, L"XXXXXXX");
	Registrator.OperationRegistration = &Operation;
	Registrator.RegistrationContext = &RegistrationContext;

	return ObRegisterCallbacks(&Registrator, handle);
}