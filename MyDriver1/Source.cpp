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

	// получаем процесс, над которым проходят манипуляции
	PEPROCESS Target = (PEPROCESS)OperationInformation->Object;

	// читаем из структуры PEPROCESS имя процесса
	DWORD32 ImageFilename = (DWORD32)Target + 0x16c;

	UCHAR imageName[15];

	// запишем его для удобства рядом
	RtlCopyMemory(imageName, (const void*)ImageFilename, 15);

	// если имя процесса calc.exe ...
	if (!strcmp((const char*)imageName, PROCESS_TO_PROTECT)) {

		// берем флаги доступа, с которыми пытаются создать handle, и если там есть PROCESS_TERMINATE, то убираем из них его же
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
	
	// устанавливаем callback на выгрузку драйвера
	DriverObject->DriverUnload = OnUnload;

	OB_CALLBACK_REGISTRATION Registrator;
	OB_OPERATION_REGISTRATION Operation;
	REG_CONTEXT RegistrationContext;

	// хотим ставить колбеки на создание хэндлов связанных с процессами
	Operation.ObjectType = PsProcessType;
	Operation.Operations = OB_OPERATION_HANDLE_CREATE;

	// ставим колбеки, один выполняется ДО операции, другой ПОСЛЕ
	Operation.PostOperation = PostOperationCallback;
	Operation.PreOperation = PreOperationCallback;

	// заполняем структуру, которая содержит в себе описание колбеков
	Registrator.Version = OB_FLT_REGISTRATION_VERSION;
	Registrator.OperationRegistrationCount = 1;
	RtlInitUnicodeString(&Registrator.Altitude, L"XXXXXXX");
	Registrator.OperationRegistration = &Operation;
	Registrator.RegistrationContext = &RegistrationContext;

	HANDLE RegHandle;

	// регистрируем колбеки
	NTSTATUS status = ObRegisterCallbacks(&Registrator, &RegHandle);

	if (!NT_SUCCESS(status)) {
		Log("Error: %p\n", status);
	} else {
		Log("Well done!\n");
	}

	Log("NEP NEP\n");
	return STATUS_SUCCESS;
}