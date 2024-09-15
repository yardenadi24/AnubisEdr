#include "Section.h"

NTSTATUS Section::Initialize(SECTION_TYPE type)
{
	UNREFERENCED_PARAMETER(type);
	NTSTATUS Status = STATUS_SUCCESS;

	RtlRunOnceInitialize(&SectionSingletonState);

	return Status;
}

NTSTATUS Section::GetSection(PDLL_STATS* ppSectionInfo)
{
	UNREFERENCED_PARAMETER(ppSectionInfo);
	NTSTATUS Status = STATUS_SUCCESS;

	ASSERT(sectionType == SEC_TP_NATIVE || sectionType == SEC_TP_WOW);

	// Singleton approach
	PVOID Context = NULL;
	Status = RtlRunOnceBeginInitialize(&SectionSingletonState, 0, &Context);
	if (Status == STATUS_PENDING)
	{
		// We get here in first init
		Context = NULL;

		DLL_STATS* pDStats = (DLL_STATS*)ExAllocatePoolWithTag(PagedPool, sizeof(DLL_STATS), TAG('kDSm'));

		if (pDStats)
		{

			// We need to trick the system into creating a KnownDll section for us
			// Using the security descriptor of the kernel32.dll section

			// Temporarily attach the current thread to the address space of the system process
			KAPC_STATE as;
			KeStackAttachProcess(PsInitialSystemProcess, &as);

			// Create our 'KnownDll section
			Status = CreateKnownDllSection(*pDStats);



		}

		// Finalize
		NTSTATUS StatusComplete = RtlRunOnceComplete(&SectionSingletonState, 0, &Context);
		if(!NT_SUCCESS(StatusComplete))
		{
			// Error on complete
			DbgPrintLine("GetSection::Error: (0x%X) RtlRunOnceComplete", StatusComplete);
			
			if (NT_SUCCESS(Status))
			{
				Status = StatusComplete;
			}
		
		}
	}
	else if (Status != STATUS_SUCCESS)
	{
		// Error
		DbgPrintLine("GetSection::Error: (0x%X) RtlRunOnceBeginInitialize", Status);
	}
	
	// If we did not succeed
	if (!Context && Status == STATUS_SUCCESS)
	{
		// We previously failed to create section
		Status = STATUS_NONEXISTENT_SECTOR;
	}

	if (ppSectionInfo)
	{
		*ppSectionInfo = (DLL_STATS*)Context;
	}

	return Status;
}