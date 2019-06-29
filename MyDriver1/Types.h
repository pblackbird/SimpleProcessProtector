#pragma once

#define PROCESS_TERMINATE 0x0001
#define PROCESS_VM_READ 0x0010
#define PROCESS_VM_OPERATION 0x0008
#define PROCESS_VM_WRITE 0x0020

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