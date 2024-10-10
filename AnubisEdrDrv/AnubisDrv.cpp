#include "AnubisDrv.h"
#include "AnbsCommons.h"
#include "AnbsList.h"
#include "AnbsProcess.h"
#include "AnbsMemory.h"

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

// Call back for every process beinng created
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
		PPROCESS_INFO pProcessInfo = (PPROCESS_INFO)AllocMemory(sizeof(PROCESS_INFO));
		if (pProcessInfo)
		{
			// Init
			NTSTATUS Status;
			if (!NT_SUCCESS(Status = pProcessInfo->Initialize(CreateInfo, ProcessId)))
			{
				DbgError("Failed initializing process info (%u)", Status);
				return;
			}
			// Add to list
			DbgInfo("Adding to process list process: (PID:%u) name: '%wZ', cmd: '%wZ'",
					HandleToULong(ProcessId),
					pProcessInfo->ImageFileName_,
					pProcessInfo->CmdLine_);
		
			DbgInfo("Using Distractor to delete object");
			pProcessInfo->FreeInternalMemory();

			DbgInfo("Using Delete to delete object");
			FreeMemory(pProcessInfo);
		}
	
	}
	else {

		DbgInfo("Process termination: PID: %u '%s'", HandleToULong(ProcessId), PsGetProcessImageFileName(Process));
	}
}