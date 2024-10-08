#include "AnubisDrv.h"


extern "C"
NTSTATUS DriverEntry(
	PDRIVER_OBJECT DriverObject,
	PUNICODE_STRING RegistryPath
)
{
	NTSTATUS Status = STATUS_SUCCESS;
	DbgInfo("Loading Anubis DrvObject 0x%p, RegPath: %wZ", DriverObject, RegistryPath);

	DriverObject->DriverUnload = AnubisUnload;
	DriverObject->MajorFunction[IRP_MJ_CREATE] = AnubisCreateClose;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = AnubisCreateClose;
	DriverObject->MajorFunction[IRP_MJ_READ] = AnubisRead;
	DriverObject->MajorFunction[IRP_MJ_WRITE] = AnubisWrite;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = AnubisIoCtl;

	return Status;
}

VOID
AnubisUnload(PDRIVER_OBJECT pDriverObject)
{
	DbgInfo("Unloading Anubis 0x%p", pDriverObject);
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