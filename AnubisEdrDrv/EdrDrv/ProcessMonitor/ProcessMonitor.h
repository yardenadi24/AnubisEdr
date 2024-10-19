#pragma once
#include <ntddk.h>
#include "../KernelCommonLibs/AnbsList.h"
#include "../Common/AnbsCommons.h"
#include "AnbsProcess.h"

namespace monitor {
namespace process {

NTSTATUS
Initialize();
		
NTSTATUS
Finalize();

// Call back for every process being created
VOID
ProcessCreationCallback(
		PEPROCESS Process,
		HANDLE ProcessId,
		PPS_CREATE_NOTIFY_INFO CreateInfo);
}
}

// NT API
extern "C"
NTKERNELAPI
PCHAR
NTAPI
PsGetProcessImageFileName(
	_In_ PEPROCESS Process
);