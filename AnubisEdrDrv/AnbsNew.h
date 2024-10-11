#pragma once
#include <ntddk.h>

#pragma warning(disable: 4996)

// Kernel implementation for memory allocation and deletion

#define ANBS_MEM_TAG 'sbnA'

enum class ANBS_POOL_TYPE {
	NonPaged,
	Paged,
	NonPagedExecute,
	NonPagedNx,
};

inline constexpr POOL_TYPE ConvertToSystemPoolType(ANBS_POOL_TYPE PoolType)
{
	switch (PoolType)
	{
	case ANBS_POOL_TYPE::NonPaged:
		return NonPagedPool;
	case ANBS_POOL_TYPE::Paged:
		return PagedPool;
	case ANBS_POOL_TYPE::NonPagedExecute:
		return NonPagedPoolExecute;
	case ANBS_POOL_TYPE::NonPagedNx:
		return NonPagedPoolNx;
	default:
		return MaxPoolType;
	}
}


// Placement new
PVOID __cdecl operator new (SIZE_T, PVOID);
PVOID __cdecl operator new[](SIZE_T, PVOID);

// Placement delete
VOID  __cdecl operator delete (PVOID, PVOID);
VOID  __cdecl operator delete[](PVOID, PVOID);

// Allocation new
PVOID __cdecl operator new (SIZE_T, POOL_TYPE PoolType);
PVOID __cdecl operator new[](SIZE_T, POOL_TYPE PoolType);

// Allocation delete
VOID __cdecl operator delete (PVOID Ptr, SIZE_T Size);
VOID __cdecl operator delete[](PVOID Ptr, SIZE_T Size);
