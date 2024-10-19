#include "AnbsNew.h"

// placement new
PVOID __cdecl operator new (SIZE_T, PVOID Ptr)
{
	return Ptr;
}

PVOID __cdecl operator new[](SIZE_T, PVOID Ptr)
{
	return Ptr;
}

// placement delete
VOID  __cdecl operator delete (PVOID, PVOID){}

VOID  __cdecl operator delete[](PVOID, PVOID){}

// Allocate new
PVOID __cdecl operator new (SIZE_T Size, POOL_TYPE PoolType)
{
	Size = (Size != 0) ? Size : 1;
	PVOID pMem = ExAllocatePoolWithTag(PoolType, Size, ANBS_MEM_TAG);
	if (pMem)
		RtlZeroMemory(pMem, Size);
	return pMem;
}

PVOID __cdecl operator new[](SIZE_T Size, POOL_TYPE PoolType)
{
	Size = (Size != 0) ? Size : 1;
	PVOID pMem = ExAllocatePoolWithTag(PoolType, Size, ANBS_MEM_TAG);
	if (pMem)
		RtlZeroMemory(pMem, Size);
	return pMem;
}

// Allocation delete
VOID __cdecl operator delete (PVOID Ptr, SIZE_T Size)
{
	UNREFERENCED_PARAMETER(Size);
	if (Ptr)
		ExFreePool(Ptr);
}

VOID __cdecl operator delete[](PVOID Ptr, SIZE_T Size)
{
	UNREFERENCED_PARAMETER(Size);
	if (Ptr)
		ExFreePool(Ptr);
}