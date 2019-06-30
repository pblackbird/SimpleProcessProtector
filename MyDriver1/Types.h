#pragma once

#define PROCESS_TERMINATE 0x0001
#define PROCESS_VM_READ 0x0010
#define PROCESS_VM_OPERATION 0x0008
#define PROCESS_VM_WRITE 0x0020

#define MM_COPY_MEMORY_PHYSICAL 0x1
#define MM_COPY_MEMORY_VIRTUAL 0x2

typedef struct _MM_COPY_ADDRESS {
	union {
		PVOID            VirtualAddress;
		PHYSICAL_ADDRESS PhysicalAddress;
	};
	union {
		PVOID            VirtualAddress;
		PHYSICAL_ADDRESS PhysicalAddress;
	};
} MM_COPY_ADDRESS, * PMMCOPY_ADDRESS;

typedef struct _SSDTHook {
	DWORD32 FunctionAddress;
	DWORD32 HookAddress;
	PDWORD32 OriginalFunctionAddress;
} SSDT_HookEntry;

const UCHAR ProhibitedAccessRights[] = {
	//PROCESS_TERMINATE,
	PROCESS_VM_READ,
	PROCESS_VM_OPERATION,
	PROCESS_VM_WRITE
};

typedef struct _SSDT
{
	PDWORD32 ServiceTable;
	PDWORD32 CounterTableBase;
	DWORD32 ServiceLimit;
	PCHAR ArgumentTable;
} SSDT;

typedef struct {
	ULONG ulIndex;
	USHORT Version;
} REG_CONTEXT;