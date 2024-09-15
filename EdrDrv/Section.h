#pragma once

#pragma once

#include <ntifs.h>
#include <minwindef.h>
#include <ntimage.h>
#include <ntstrsafe.h>

#include "EdrDrvCommon.h"

#define DRIVER_PREFIX "EdrDrv: "
#define DbgPrintLine(s,...) DbgPrint(DRIVER_PREFIX s "\n", __VA_ARGS__)
#define TAG(t) ( ((((ULONG)t) & 0xFF) << (8 * 3)) | ((((ULONG)t) & 0xFF00) << (8 * 1)) | ((((ULONG)t) & 0xFF0000) >> (8 * 1)) | ((((ULONG)t) & 0xFF000000) >> (8 * 3)) )

enum SECTION_TYPE
{
	SEC_TP_NATIVE, 
	SEC_TP_WOW,
};

typedef struct _DLL_STATS
{
	SECTION_TYPE secType;
	PVOID Section;

	BOOLEAN IsValid()
	{
		return Section != NULL;
	}

}DLL_STATS, *PDLL_STATS;

class Section
{
private:
	SECTION_TYPE sectionType;
	RTL_RUN_ONCE SectionSingletonState;
public:
	NTSTATUS Initialize(SECTION_TYPE type);
	NTSTATUS GetSection(PDLL_STATS* ppSectionInfo);
};

