#pragma once

#include <ntifs.h>
#include "Source.h"

VOID InitSSDTHooker();
DWORD32 HookSSDT(DWORD32 function, DWORD32 hook);