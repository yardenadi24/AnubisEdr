#include "ProcessMonitor.h"

namespace monitor {
namespace process {

List<PROCESS_INFO>* g_pProcessList;
FAST_MUTEX g_Mutex;
SIZE_T g_MaxProcess;

NTSTATUS
Initialize()
{
	DbgInfo("Starting process monitor");
		
	NTSTATUS Status = STATUS_SUCCESS;
			
	if (g_pCommonData->ProcMonStarted_.load())
	{
		DbgError("Process monitor already initialized");
		return STATUS_ALREADY_REGISTERED;
	}
			
	g_pProcessList = new (NonPagedPool) List<PROCESS_INFO>(&g_Mutex);
	g_MaxProcess = MAX_PROCESSES;

	Status = PsSetCreateProcessNotifyRoutineEx(&ProcessCreationCallback, FALSE);
	if (!NT_SUCCESS(Status))
	{
		DbgError("Failed settings process notify");
		g_pProcessList->FreeList();
		delete g_pProcessList;
		return Status;
	}

}

NTSTATUS
Finalize()
{
	NTSTATUS Status = STATUS_SUCCESS;
	Status = PsSetCreateProcessNotifyRoutineEx(ProcessCreationCallback, TRUE);
	if (!NT_SUCCESS(Status))
	{
		DbgError("Failed removing process notify");
	}
	g_pProcessList->FreeList();
	delete g_pProcessList;
	return Status;
}


// Call back for every process being created
VOID
ProcessCreationCallback(
		PEPROCESS Process,
		HANDLE ProcessId,
		PPS_CREATE_NOTIFY_INFO CreateInfo)
{

	if (CreateInfo)
	{
		DbgInfo("Process creation: PID: %u '%s'", HandleToULong(ProcessId), PsGetProcessImageFileName(Process));

		// Allocate non paged memory for the process info struct
		PPROCESS_INFO pProcessInfo = new (NonPagedPool) _PROCESS_INFO(CreateInfo, ProcessId);
		if (pProcessInfo)
		{
			// Add to list
			DbgInfo("Adding to process list process (%u/%u): (PID:%u) name: '%wZ', cmd: '%wZ'",
				g_pProcessList->GetSize(),
				MAX_PROCESSES,
				HandleToULong(ProcessId),
				pProcessInfo->ImageFileName_,
				pProcessInfo->CmdLine_);

			if (!g_pProcessList->Add(pProcessInfo, HandleToULong(ProcessId)))
			{
				DbgError("Failed adding process to list");
				pProcessInfo->FreeProcessInfo();
				delete pProcessInfo;
			}

		}

	}
	else {
				
		PPROCESS_INFO pProcessInfo = g_pProcessList->GetDataById(HandleToULong(ProcessId));
				
		if (pProcessInfo == NULL)
		{
			DbgError("Couldn't find process id in list to remove it. (%u)", HandleToULong(ProcessId));
			return;
		}

		DbgInfo("Terminated: Removing process list process (%u/%u): (PID:%u) name: '%wZ', cmd: '%wZ'",
			g_pProcessList->GetSize(),
			MAX_PROCESSES,
			HandleToULong(ProcessId),
			pProcessInfo->ImageFileName_,
			pProcessInfo->CmdLine_);

		g_pProcessList->RemoveByData(pProcessInfo);
	}
}

}
}