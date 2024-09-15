// Driver entry cpp file

#include "EdrDrv.h"
#include "Section.h"

Section gSec;

extern "C"
NTSTATUS
NTAPI
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	UNREFERENCED_PARAMETER(DriverObject);
	UNREFERENCED_PARAMETER(RegistryPath);
	DbgPrintLine("Driver load (0x%p, %wZ)", DriverObject, RegistryPath);

	NTSTATUS Status = STATUS_SUCCESS;
	DriverObject->DriverUnload = EdrDriverUnload;

	do {
		// Set image load notification
		Status = PsSetLoadImageNotifyRoutine(EdrImageLoadNotifyRoutine);
		if (!NT_SUCCESS(Status))
		{
			DbgPrintLine("Failed to register for image load notification (0x%X)", Status);
			break;
		}
		gRegisteredForImageLoadNotifications = TRUE;

	}while(FALSE);

	return Status;
}

VOID
EdrImageLoadNotifyRoutine(
	_In_opt_ PUNICODE_STRING FullImageName,
	_In_ HANDLE ProcessId,                // pid into which image is being mapped
	_In_ PIMAGE_INFO ImageInfo
)
{
	//NTSTATUS Status = STATUS_SUCCESS;

	ASSERT(FullImageName);
	ASSERT(ImageInfo);

	UNREFERENCED_PARAMETER(ImageInfo);
	STATIC_UNICODE_STRING(Kernel32DllName,"\\kernel32.dll");
	// We are looking for kernel32.dll only
	if (
		!ImageInfo->SystemModeImage &&									// Skip anything mapped to kernel
		ProcessId == PsGetCurrentProcessId() &&							// Skip sections mapped remotely
		RtlSuffixUnicodeString(&Kernel32DllName,FullImageName,TRUE) &&	// Check that it is kernel32.dll
		IsMappedByLdrLoadDll((PUNICODE_STRING)&Kernel32DllName)			// Check if loaded by LdrLoadDll
#if defined(_DEBUG)		
		&& IsSpecificProcessW(ProcessId,L"notepad.exe", FALSE)				// For debugging limit for specific processes
#endif		
		)
	{	
		// We can proceed with injection
		DbgPrintLine("TEST -- Image load for pid = %u \"%wZ\"", HandleToUlong(ProcessId), FullImageName);
		
	}

}

BOOLEAN IsSpecificProcessW(HANDLE ProcessId, const WCHAR* ImageName, BOOLEAN IsDebugged)
{
	BOOLEAN Ret = FALSE;
	UNREFERENCED_PARAMETER(ProcessId);
	UNREFERENCED_PARAMETER(ImageName);
	UNREFERENCED_PARAMETER(IsDebugged);
	ASSERT(ImageName);
	NTSTATUS Status = STATUS_SUCCESS;
	PEPROCESS Process;
	Status = PsLookupProcessByProcessId(ProcessId, &Process);
	if (!NT_SUCCESS(Status))
	{
		DbgPrintLine("IsSpecificProcessW:: Failed to get process by id.(pid: %u, status: 0x%X)", HandleToUlong(ProcessId), Status);
		return Ret;
	}
	// Check for kernel debugger
	if (!IsDebugged ||
		PsIsProcessBeingDebugged(Process)
		)
	{
		// Get Process handle
		HANDLE hProc;
		if (ObOpenObjectByPointer(Process, OBJ_KERNEL_HANDLE, NULL,
			PROCESS_ALL_ACCESS, *PsProcessType, KernelMode, &hProc) == STATUS_SUCCESS)
		{
			// Get proc name
			WCHAR buff[GCPFN_BUFF_SIZE];
			UNICODE_STRING* puStr = (UNICODE_STRING*)buff;
			PWCH pWBuff = (PWCH)((BYTE*)buff + sizeof(UNICODE_STRING));
			puStr->Length = 0;
			puStr->MaximumLength = (USHORT)(sizeof(buff) - sizeof(UNICODE_STRING));
			puStr->Buffer = pWBuff;

			if (ZwQueryInformationProcess(hProc, ProcessImageFileName, puStr, sizeof(buff), NULL) == STATUS_SUCCESS)
			{
				// Safety null
				*(WCHAR*)((BYTE*)buff + sizeof(buff) - sizeof(WCHAR)) = 0;

				// Make sure we have null terminated string
				if (puStr->Length + sizeof(WCHAR) <= puStr->MaximumLength)
				{
					*(WCHAR*)((BYTE*)puStr->Buffer + puStr->Length) = 0;
				}

				// Find file name
				WCHAR* pLastSlash = NULL;
				for (WCHAR* pWc = pWBuff;; pWc++)
				{
					WCHAR wc = *pWc;
					if (!wc)
					{
						if (pLastSlash)
						{
							// Use the next wchar
							pWBuff = pLastSlash + 1;
						}
						break;
					}
					else if (wc == L'\\') {
						pLastSlash = pWc;
					}
				}

				// Compare to the provided name
				if (_wcsicmp(ImageName, pWBuff) == 0)
				{
					Ret = TRUE;
				}
			}
			// Close
			ZwClose(hProc);
		}
	}

	// By calling PsLookup we increased the ref count, so we need to decrease it back
	ObDereferenceObject(Process);

	return Ret;
}

BOOLEAN IsMappedByLdrLoadDll(PUNICODE_STRING ModuleName)
{
	BOOLEAN Ret = FALSE;
	UNICODE_STRING Name;
	//DbgPrintLine("Checking is mapped by LdrLoadDll :\"%wZ\"", ModuleName);
	__try {

		PNT_TIB Teb = (PNT_TIB)PsGetCurrentThreadTeb();
		if (
			!Teb ||
			!Teb->ArbitraryUserPointer // ArbitraryUserPointer is set by LdrLoadDll()
			)
		{
			// This is not it
			return Ret;
		}

		// If the LdrLoadDll is used this will contain the name of the module
		Name.Buffer = (PWSTR)Teb->ArbitraryUserPointer;

		// Check this is a valid user mode address - if not succeed will throw exception
		ProbeForRead(Name.Buffer, sizeof(WCHAR), __alignof(WCHAR));

		Name.Length = (USHORT)wcsnlen(Name.Buffer, MAXSHORT);
		if (Name.Length == MAXSHORT)
		{
			// Name is too long
			return FALSE;
		}

		// Adjust length to bytes
		Name.Length *= sizeof(WCHAR);
		Name.MaximumLength = Name.Length;

		// Check if out needed module
		Ret = RtlSuffixUnicodeString(ModuleName, &Name, TRUE);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		DbgPrintLine("Exception: EdrImageLoadNotifyRoutine (0x%X)", GetExceptionCode());
	}

	return Ret;
}

VOID
NTAPI 
EdrDriverUnload(PDRIVER_OBJECT DriverObject)
{
	NTSTATUS Status = STATUS_SUCCESS;
	if (gRegisteredForImageLoadNotifications)
		Status = PsRemoveLoadImageNotifyRoutine(EdrImageLoadNotifyRoutine);
	if (!NT_SUCCESS(Status))
	{
		DbgPrintLine("Failed to remove notification for image load (0x%X)", Status);
	}
	DbgPrintLine("Driver unload (0x%p), status: %u", DriverObject, Status);

}