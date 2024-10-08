#pragma once
#include <ntddk.h>

#define ANUBIS_PREFIX "Anubis: "

// Some print macros
#define DbgInfo(s,...) DbgPrint(ANUBIS_PREFIX "<Info>" "::%s:: " s "\n", __FUNCTION__, __VA_ARGS__)
#define DbgError(s,...) DbgPrint(ANUBIS_PREFIX "<Error>" "::%s:: "  s "\n", __FUNCTION__, __VA_ARGS__)
#define DbgWarning(s,...) DbgPrint(ANUBIS_PREFIX "<Warning>" "::%s:: " s "\n", __FUNCTION__, __VA_ARGS__)

// Memory tags
// TODO: Move to a dedicated memory managing file
#define ANUBIS_MEM_TAG 'sbnA'

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