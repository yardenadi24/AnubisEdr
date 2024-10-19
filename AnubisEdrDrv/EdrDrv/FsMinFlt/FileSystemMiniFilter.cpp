#include "FileSystemMiniFilter.h"


PFLT_FILTER gFilterHandle;
ULONG_PTR OperationStatusCtx = 1;

CONST FLT_OPERATION_REGISTRATION Callbacks[] = {
    { IRP_MJ_CREATE,
      0,
      MyPreCreateOperation,
      MyPostCreateOperation },
    { IRP_MJ_OPERATION_END }
};

CONST FLT_REGISTRATION FilterRegistration = {
    sizeof(FLT_REGISTRATION),         // Size
    FLT_REGISTRATION_VERSION,         // Version
    0,                                // Flags
    NULL,                             // Context
    Callbacks,                        // Operation callbacks
    NULL,                             // Unload
    NULL,                             // InstanceSetup
    NULL,                             // InstanceQueryTeardown
    NULL,                             // InstanceTeardownStart
    NULL,                             // InstanceTeardownComplete
    NULL,                             // GenerateFileName
    NULL,                             // GenerateDestinationFileName
    NULL                              // NormalizeNameComponent
};

NTSTATUS DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
)
{
    NTSTATUS status;

    status = FltRegisterFilter(DriverObject, &FilterRegistration, &gFilterHandle);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    // Start filtering I/O operations
    status = FltStartFiltering(gFilterHandle);
    if (!NT_SUCCESS(status)) {
        FltUnregisterFilter(gFilterHandle);
    }

    return status;
}

NTSTATUS FLTAPI MyPreCreateOperation(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID* CompletionContext
)
{
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(CompletionContext);

    // Add EDR-related pre-create checks (e.g., monitoring for suspicious file creations)
    PFLT_IO_PARAMETER_BLOCK iopb = Data->Iopb;
    if (iopb->MajorFunction == IRP_MJ_CREATE) {
        // Example: Check file path and log or block if malicious
        UNICODE_STRING targetFileName = iopb->TargetFileObject->FileName;
        // Detect suspicious behavior
    }

    return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}

NTSTATUS FLTAPI MyPostCreateOperation(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
)
{
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(CompletionContext);
    UNREFERENCED_PARAMETER(Flags);

    // Add EDR-related post-create handling (e.g., flagging or quarantining suspicious files)
    // Can analyze file contents here

    return FLT_POSTOP_FINISHED_PROCESSING;
}
