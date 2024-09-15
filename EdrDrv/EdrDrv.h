#pragma once

#include <ntifs.h>
#include <minwindef.h>
#include <ntimage.h>
#include <ntstrsafe.h>

#include "EdrDrvCommon.h"

#define DRIVER_PREFIX "EdrDrv: "

#define GCPFN_BUFF_SIZE (sizeof(UNICODE_STRING) + (MAX_PATH + 1)*sizeof(WCHAR))

// Undocumented functions
///////////////////////////////////////////////////
extern "C"
{
    __declspec(dllimport) BOOLEAN PsIsProcessBeingDebugged(PEPROCESS Process);
    __declspec(dllimport) NTSTATUS ZwQueryInformationProcess
    (
        IN HANDLE ProcessHandle,
        IN PROCESSINFOCLASS ProcessInformationClass,
        OUT PVOID ProcessInformation,
        IN  ULONG ProcessInformationLength,
        OUT PULONG ReturnLength OPTIONAL
    );
}
///////////////////////////////////////////////////

// Debug build def
#ifdef DBG
#define _DEBUG DBG
#define VERIFY(f) ASSERT(f)
#else
#define VERIFY(f) ((void)(f))
#endif

#define DbgPrintLine(s,...) DbgPrint(DRIVER_PREFIX s "\n", __VA_ARGS__)

static BOOL gRegisteredForImageLoadNotifications = FALSE;

// Static variables macros
#define echo(x) x
#define label(x) echo(x)##__LINE__
#define RTL_CONSTANT_STRINGW_(s) {sizeof(s) - sizeof(s[0]), sizeof(s), const_cast<PWSTR>(s)}

// -- Static unicode string
#define STATIC_UNICODE_STRING(name, str) \
static const WCHAR label(__)[] = echo(L)str;\
static const UNICODE_STRING name = RTL_CONSTANT_STRINGW_(label(__))

// -- Static object attributes
#define STATIC_OBJECT_ATTRIBUTES(oa, name)\
    STATIC_UNICODE_STRING(label(m), name);\
    static OBJECT_ATTRIBUTES oa = {sizeof(oa), 0, const_cast<PUNICODE_STRING>(&label(m)), OBJ_CASE_INSENSITIVE}

// Check if this thread runs within LdLoadDll() function for the module 'ModuleName'
// Otherwise the call could have came from invoking ZwMapViewOfSection with SEC_IMAGE
BOOLEAN IsMappedByLdrLoadDll(PUNICODE_STRING ModuleName);

// Mainly for debugging: Check if process with ProcessId is a specific process by its file name
BOOLEAN IsSpecificProcessW(HANDLE ProcessId, const WCHAR* ImageName, BOOLEAN bIsDebugged);

VOID
NTAPI
EdrDriverUnload(PDRIVER_OBJECT DriverObject);

VOID
EdrImageLoadNotifyRoutine(
    _In_opt_ PUNICODE_STRING FullImageName,
    _In_ HANDLE ProcessId,                // pid into which image is being mapped
    _In_ PIMAGE_INFO ImageInfo
    );