#include "AnubisDrv.h"
#include "./Common/AnbsCommons.h"
#include "./KernelCommonLibs/AnbsList.h"
#include "./KernelCommonLibs/AnbsNew.h"
#include "./ProcessMonitor/AnbsProcess.h"
#include "./ProcessMonitor/ProcessMonitor.h"
#include "./Ioctl/Ioctl.h"

SIZE_T g_MaxProcess;
FAST_MUTEX g_Mutex;
PANUBIS_COMMON_GLOBAL_DATA g_pCommonData;

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

		// Process monitoring
		Status = monitor::process::Initialize();
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

		DriverObject->Flags |= DO_BUFFERED_IO;

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
	
	monitor::process::Finalize();

	IoDeleteDevice(pDriverObject->DeviceObject);

	UNICODE_STRING SymlinkName = RTL_CONSTANT_STRING(DEVICE_SYMLINK_NAME);
	NTSTATUS Status = IoDeleteSymbolicLink(&SymlinkName);
	if (!NT_SUCCESS(Status))
	{
		DbgError("Failed deleting Anubis symbolic link (0x%u).", Status);
	}
	
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

	NTSTATUS Status = STATUS_SUCCESS;

	PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(pIrp);

	ULONG Info = 0;
	
	auto ReadParams = IrpSp->Parameters.Read;

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

