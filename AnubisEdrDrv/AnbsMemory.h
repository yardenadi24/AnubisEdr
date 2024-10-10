#pragma once
#include <ntddk.h>
#include "AnbsCommons.h"

#pragma warning(disable : 4996)

PVOID AllocMemory(SIZE_T Size)
{
	return ExAllocatePoolWithTag(NonPagedPool, Size, ANUBIS_MEM_TAG);
}

VOID FreeMemory(PVOID address)
{
	ExFreePoolWithTag(address, ANUBIS_MEM_TAG);
}