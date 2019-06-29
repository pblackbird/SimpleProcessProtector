#include "SSDT.h"

unsigned long* mappedSSDT;
MDL* ssdtMdl;

extern "C" __declspec(dllimport) SSDT KeServiceDescriptorTable;

#define SYSTEMSERVICE(Function) \
	KeServiceDescriptorTable.ServiceTable[*(PULONG)((PUCHAR)Function+1)]

#define SYSCALL_INDEX(Function) *(PULONG)((PUCHAR)Function+1)

VOID InitSSDTHooker() {
	ssdtMdl = MmCreateMdl(0, KeServiceDescriptorTable.ServiceTable,
		KeServiceDescriptorTable.ServiceLimit * 4);

	MmBuildMdlForNonPagedPool(ssdtMdl);
	ssdtMdl->MdlFlags |= MDL_MAPPED_TO_SYSTEM_VA;

	mappedSSDT = (unsigned long*)MmMapLockedPages(ssdtMdl, KernelMode);
}

DWORD32 HookSSDT(DWORD32 function, DWORD32 hook) {
	DWORD32 original = (DWORD32)_InterlockedExchange((volatile LONG*)& mappedSSDT[SYSCALL_INDEX(function)], \
		(ULONG)hook);

	return original;
}