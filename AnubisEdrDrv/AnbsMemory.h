#pragma once
#include <ntddk.h>
#include "AnbsCommons.h"

PVOID operator new(SIZE_T Size)
{
	PVOID Ptr = ExAllocatePoolWithTag(NonPagedPool, Size, ANUBIS_MEM_TAG);
	if (Ptr != NULL)
	{
		RtlZeroMemory(Ptr,Size);
	}
	return Ptr;
}

VOID operator delete(PVOID Ptr)
{
	if (Ptr != NULL)
	{
		ExFreePoolWithTag(Ptr, ANUBIS_MEM_TAG);
	}
}