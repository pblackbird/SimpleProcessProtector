#pragma once

#include <ntifs.h>
#include <stdarg.h>

#define PROCESS_TERMINATE 0x0001

typedef struct {
	ULONG ulIndex;
	USHORT Version;
} REG_CONTEXT;