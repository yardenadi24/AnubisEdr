#pragma once
#include <ntddk.h>

// Device related defines
#define DEVICE_NAME L"\\Device\\AnubisDevice"
#define DEVICE_SYMLINK_NAME L"\\??\\AnubisDevice"

// Unload routine for the driver
VOID
AnubisUnload(PDRIVER_OBJECT pDriverObject);

// Major functions
NTSTATUS
AnubisCreateClose(PDEVICE_OBJECT pDeviceObject, PIRP pIrp);

NTSTATUS
AnubisRead(PDEVICE_OBJECT pDeviceObject, PIRP pIrp);

NTSTATUS
AnubisWrite(PDEVICE_OBJECT pDeviceObject, PIRP pIrp);

NTSTATUS
AnubisIoCtl(PDEVICE_OBJECT pDeviceObject, PIRP pIrp);

NTSTATUS
CompleteIoRequest(PIRP Irp,
	NTSTATUS status = STATUS_SUCCESS,
	ULONG_PTR written = 0);