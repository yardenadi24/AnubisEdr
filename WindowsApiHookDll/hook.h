#pragma once
#include "Windows.h"

// The detour function to which we redirect calls
// to MessageBoxA on hook
int
WINAPI
DetourMessageBoxA(_In_opt_ HWND hWnd,
	_In_opt_ LPCSTR lpText,
	_In_opt_ LPCSTR lpCaption,
	_In_ UINT uType);

// Install a hook in pTarget function
// to pDetour function
BOOL
WINAPI
InstallHook(
	LPVOID pTarget,
	LPVOID pDetour,
	LPVOID* ppOriginal
);

// Installs the hook for MessageBoxA
BOOL InstallMessageBoxAHook();

// Return the address of MessageBoxA function
FARPROC GetMessageBoxAAddress();


