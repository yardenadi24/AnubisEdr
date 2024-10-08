#include "AnubisDrv.h"


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
			FALSE,
			&pDeviceObj);

		if (!NT_SUCCESS(Status))
		{
			DbgError("Failed creating Anubis device (0x%u).", Status);
			break;
		}

		// Create the SymLink
		UNICODE_STRING SymlinkName = RTL_CONSTANT_STRING(DEVICE_SYMLINK_NAME);

		Status = IoCreateSymbolicLink(&SymlinkName, &DeviceName);
		if (!NT_SUCCESS(Status))
		{
			DbgError("Failed creating Anubis symbolic link (0x%u).", Status);
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
}

NTSTATUS
CompleteIoRequest(PIRP Irp, NTSTATUS status, ULONG_PTR written)
{
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = written;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}

NTSTATUS
AnubisCreateClose(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
	UNREFERENCED_PARAMETER(pDeviceObject);
	return CompleteIoRequest(pIrp);
}

NTSTATUS
AnubisRead(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
	UNREFERENCED_PARAMETER(pDeviceObject);
	return CompleteIoRequest(pIrp);
}

NTSTATUS
AnubisWrite(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
	UNREFERENCED_PARAMETER(pDeviceObject);
	return CompleteIoRequest(pIrp);
}

NTSTATUS
AnubisIoCtl(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
	UNREFERENCED_PARAMETER(pDeviceObject);
	return CompleteIoRequest(pIrp);
}