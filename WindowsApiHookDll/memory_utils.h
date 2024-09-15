#pragma once
#include <Windows.h>

// Size of each memory block 1MB
#define BLOCK_SIZE 0x1000

// Max range for seeking a memory block 1GB
#define MAX_MEMORY_RANGE 0x40000000

// Memory protection flag for executable address
#define PAGE_EXECUTE_FLAGS   (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)

// Allocate buffer in range for relative jump from the pOrigin
ULONG_PTR AllocateBuffer(LPVOID pOrigin);

