#include "pch.h"
#include "memory_utils.h"
#include <stdio.h>

ULONG_PTR AllocateBuffer(LPVOID pOrigin)
{
	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);

	// Page size (usually 4 KB on most systems)
	SIZE_T pageSize = sysInfo.dwPageSize;

	// Allocation granularity (usually 64 KB)
	SIZE_T allocationGranularity = sysInfo.dwAllocationGranularity;

	ULONG_PTR minAddr = (ULONG_PTR)sysInfo.lpMinimumApplicationAddress;
	ULONG_PTR maxAddr = (ULONG_PTR)sysInfo.lpMaximumApplicationAddress;

	// Adjust lower boundary
	if ((ULONG_PTR)pOrigin > MAX_MEMORY_RANGE &&
		minAddr < (ULONG_PTR)pOrigin - MAX_MEMORY_RANGE)
	{
		minAddr = (ULONG_PTR)pOrigin - MAX_MEMORY_RANGE;
	}

	// Adjust upper boundary
	if (maxAddr > (ULONG_PTR)pOrigin + MAX_MEMORY_RANGE)
	{
		maxAddr = (ULONG_PTR)pOrigin + MAX_MEMORY_RANGE;
	}

	// Allocating in ascending order so we need to make 
	// room for a the block size
	maxAddr -= BLOCK_SIZE - 1;

	// Allocate the block
	ULONG_PTR tryAddr = (ULONG_PTR)pOrigin;

	// Round down
	tryAddr -= tryAddr % allocationGranularity;

	ULONG_PTR tryAddrBelow = tryAddr - allocationGranularity;
	ULONG_PTR tryAddrAbove = tryAddr + allocationGranularity;

	ULONG_PTR allocatedAddr = NULL;

	while (tryAddrBelow > minAddr || tryAddrAbove < maxAddr)
	{
		// Try below
		allocatedAddr = (ULONG_PTR)VirtualAlloc((LPVOID)tryAddrBelow, pageSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
		if (allocatedAddr)
		{
			return allocatedAddr;
		}
		tryAddrBelow -= allocationGranularity;
		DWORD error = GetLastError();
		if (error != 0) {
			printf("VirtualAlloc failed with error code: %lu\n", error);
		}
		// We could not allocate below, try above
		allocatedAddr = (ULONG_PTR)VirtualAlloc((LPVOID)tryAddrAbove, pageSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
		if (allocatedAddr)
		{
			return allocatedAddr;
		}
		tryAddrAbove += allocationGranularity;
		error = GetLastError();
		if (error != 0) {
			printf("VirtualAlloc failed with error code: %lu\n", error);
		}
	}

	return NULL;

}