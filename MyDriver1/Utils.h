#pragma once

#include <ntifs.h>

VOID GetImageNameFromProcess(_In_ PEPROCESS Target, _Out_ UCHAR* output);
NTSTATUS InitObCallbacks(
	_In_ POB_PRE_OPERATION_CALLBACK preOperation,
	_In_ POB_POST_OPERATION_CALLBACK postOperation,
	_Out_ PHANDLE handle
);