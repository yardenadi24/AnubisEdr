#include "AnubisDrv.h"
#include "AnbsCommons.h"
#include "AnbsList.h"
#include "AnbsNew.h"
#include "AnbsProcess.h"

List<PROCESS_INFO>* g_pProcessList;
SIZE_T g_MaxProcess;
FAST_MUTEX g_Mutex;

// Driver entry
// TODO:: Move main logic to dedicated initialization function
extern "C"
NTSTATUS DriverEntry(
	PDRIVER_OBJECT DriverObject,
	PUNICODE_STRING RegistryPath
)
{
	NTSTATUS Status = STATUS_SUCCESS;
	DbgInfo("Loading Anubis DrvObject 0x%p, RegPath: %wZ", DriverObject, RegistryPath);

	do {

		// Create device
		UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(DEVICE_NAME);
		PDEVICE_OBJECT pDeviceObj;

		Status = IoCreateDevice(DriverObject,
			0,
			&DeviceName,
			FILE_DEVICE_UNKNOWN,
			0,
			TRUE,
			&pDeviceObj);

		if (!NT_SUCCESS(Status))
		{
			DbgError("Failed creating Anubis device (0x%u).", Status);
			break;
		}

		// Create the Symbolic link
		UNICODE_STRING SymlinkName = RTL_CONSTANT_STRING(DEVICE_SYMLINK_NAME);

		Status = IoCreateSymbolicLink(&SymlinkName, &DeviceName);
		if (!NT_SUCCESS(Status))
		{
			DbgError("Failed creating Anubis symbolic link (0x%u).", Status);
			break;
		}

		// Process creation notification
		Status = PsSetCreateProcessNotifyRoutineEx2(PsCreateProcessNotifySubsystems, ProcessCreationCallback, FALSE);
		if (!NT_SUCCESS(Status))
		{
			IoDeleteDevice(pDeviceObj);
			IoDeleteSymbolicLink(&SymlinkName);
			DbgError("Failed setting notify routine (0x%u).", Status);
			break;
		}

		DriverObject->DriverUnload = AnubisUnload;
		DriverObject->MajorFunction[IRP_MJ_CREATE] = AnubisCreateClose;
		DriverObject->MajorFunction[IRP_MJ_CLOSE] = AnubisCreateClose;
		DriverObject->MajorFunction[IRP_MJ_READ] = AnubisRead;
		DriverObject->MajorFunction[IRP_MJ_WRITE] = AnubisWrite;
		DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = AnubisIoCtl;

	} while (false);

	if (!NT_SUCCESS(Status))
	{
		DbgError("Failed loading Anubis driver (0x%u).", Status);
	}

	g_pProcessList = new (NonPagedPool) List<PROCESS_INFO>(&g_Mutex);
	g_MaxProcess = MAX_PROCESSES;

	DbgInfo("Allocated process list(MAX = %u) in : (0x%p)",g_MaxProcess ,g_pProcessList);
	return Status;
}


// Unload driver
// TODO:: Create dedicated clean up function
VOID
AnubisUnload(PDRIVER_OBJECT pDriverObject)
{
	DbgInfo("Unloading Anubis 0x%p", pDriverObject);

	IoDeleteDevice(pDriverObject->DeviceObject);

	UNICODE_STRING SymlinkName = RTL_CONSTANT_STRING(DEVICE_SYMLINK_NAME);
	NTSTATUS Status = IoDeleteSymbolicLink(&SymlinkName);
	if (!NT_SUCCESS(Status))
	{
		DbgError("Failed deleting Anubis symbolic link (0x%u).", Status);
	}

	// Process creation notification
	PsSetCreateProcessNotifyRoutineEx2(PsCreateProcessNotifySubsystems, ProcessCreationCallback, TRUE);
	
	g_pProcessList->FreeList();
	delete g_pProcessList;
}

// Complete IO request packet
NTSTATUS
CompleteIoRequest(PIRP Irp, NTSTATUS status, ULONG_PTR written)
{
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = written;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}

// Will be called on Create/Close calls
NTSTATUS
AnubisCreateClose(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
	UNREFERENCED_PARAMETER(pDeviceObject);
	return CompleteIoRequest(pIrp);
}

//  Will be called on Read calls
NTSTATUS
AnubisRead(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
	UNREFERENCED_PARAMETER(pDeviceObject);
	return CompleteIoRequest(pIrp);
}

//  Will be called on Write calls
NTSTATUS
AnubisWrite(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
	UNREFERENCED_PARAMETER(pDeviceObject);
	return CompleteIoRequest(pIrp);
}

// Will be called on IO Control requests
NTSTATUS
AnubisIoCtl(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
	UNREFERENCED_PARAMETER(pDeviceObject);
	return CompleteIoRequest(pIrp);
}

// Call back for every process being created
VOID
ProcessCreationCallback(
	PEPROCESS Process,
	HANDLE ProcessId,
	PPS_CREATE_NOTIFY_INFO CreateInfo
)
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

			//DbgInfo("Using Delete to delete object");
			//pProcessInfo->FreeProcessInfo();
			//delete pProcessInfo;
		}
	
	}
	else {
		PPROCESS_INFO pProcessInfo = g_pProcessList->GetDataById(HandleToULong(ProcessId));
		if (pProcessInfo == NULL)
		{
			DbgError("Couldnt find process id in list to remove it. (%u)", HandleToULong(ProcessId));
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