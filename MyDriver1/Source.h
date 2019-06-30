#pragma once

#include <ntifs.h>
#include <ntddk.h>
#include <ntdef.h>

#include "Types.h"

//NTKERNELAPI NTSTATUS MmCopyMemory(PVOID, MM_COPY_ADDRESS, SIZE_T, ULONG, PSIZE_T);

#define TOTAL_HOOKS 1