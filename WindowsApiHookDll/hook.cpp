#include "pch.h"
#include "hook.h"
#include "trampoline.h"
#include "memory_utils.h"
#include <stdio.h>

typedef int (WINAPI* messageBoxA)(
	_In_opt_ HWND hWnd,
	_In_opt_ LPCSTR lpText,
	_In_opt_ LPCSTR lpCaption,
	_In_ UINT uType);

messageBoxA pMessageBoxA = NULL;

typedef BOOL(WINAPI* messageBeep)(
	UINT uType);

messageBeep pMessageBeep = NULL;

BOOL
WINAPI
DetourMessageBeep(
	UINT uType
)
{
	pMessageBoxA(NULL, "I hooked Beep!, I will beep 5 times", "Hooked beep", MB_OK);
	// Beep 5 times with a delay between each beep
	for (int i = 0; i < 5; i++) {
		pMessageBeep(MB_ICONHAND);  // Call the original MessageBeep
		Sleep(3000);  // Add a 300ms delay between beeps (adjust the time as necessary)
	}
	return TRUE;
}

int WINAPI DetourMessageBoxA(_In_opt_ HWND hWnd,
	_In_opt_ LPCSTR lpText,
	_In_opt_ LPCSTR lpCaption,
	_In_ UINT uType)
{
	return pMessageBoxA(hWnd, "This is an hooked message box >:-)\n", "Hooked!", uType);
}

FARPROC GetMessageBeepAddress() {
	HMODULE hUser32 = LoadLibraryA("user32.dll");
	if (!hUser32) {
		return nullptr;
	}
	FARPROC pMessageBeep = GetProcAddress(hUser32, "MessageBeep");
	if (!pMessageBeep) {
		FreeLibrary(hUser32);
		return nullptr;
	}
	return pMessageBeep;
}

FARPROC GetMessageBoxAAddress() {
	HMODULE hUser32 = LoadLibraryA("user32.dll");
	if (!hUser32) {
		return nullptr;
	}
	FARPROC pMessageBoxA = GetProcAddress(hUser32, "MessageBoxA");
	if (!pMessageBoxA) {
		FreeLibrary(hUser32);
		return nullptr;
	}
	return pMessageBoxA;
}

BOOL InstallMessageBeepAHook()
{
	LPVOID pGetMessageBeepTarget = GetMessageBeepAddress();
	if (!pGetMessageBeepTarget)
	{
		return FALSE;
	}

	return InstallHook(pGetMessageBeepTarget, DetourMessageBeep, (LPVOID*)&pMessageBeep);
}

BOOL InstallMessageBoxAHook()
{
	LPVOID pMessageBoxATarget = GetMessageBoxAAddress();
	if (!pMessageBoxATarget)
	{
		return FALSE;
	}

	return InstallHook(pMessageBoxATarget, DetourMessageBoxA, (LPVOID*)&pMessageBoxA);
}

BOOL WINAPI InstallHook(
	LPVOID pTarget,
	LPVOID pDetour,
	LPVOID* ppOriginal
)
{
	BOOL Status = TRUE;

	// Spinlock

	// Allocate memory so we can write the trampoline into it.
	LPVOID pBuffer = (LPVOID)AllocateBuffer(pTarget);
	if (pBuffer != NULL)
	{
		TRAMPOLINE ct;

		// Set initial values before calling 'CreateTrampolineFunction'
		// pTarget -> The function we want to hook on
		// pDetour -> The function that will be executed instead
		// pBuffer -> Memory to which write the trampoline code
		ct.pTarget = pTarget;
		ct.pDetour = pDetour;
		ct.pTrampoline = pBuffer;
		if (CreateTrampolineFunction(&ct))
		{
			// At this point the trampoline contains:
			// pDetour -> the function to which we will jump on call
			// pRelay -> absolute jmp to the Detour
			// Trampoline -> instructions stolen from the target
			// that ends with a jump back
			DWORD oldProtect;
			SIZE_T patchSize = sizeof(JMP_REL);
			LPBYTE pPatchTarget = (LPBYTE)ct.pTarget;

			if (ct.patchAbove)
			{
				// Patch above means we dont have enough space for
				// rel jump in the target function so we install
				// short jump in the target function to the patch above area
				// from which we perform rel jump to the relay function
				pPatchTarget -= sizeof(JMP_REL); // Point to the patch above area
				patchSize += sizeof(JMP_REL_SHORT); // We need to add in this case a short jump
			}

			if (!VirtualProtect(pPatchTarget, patchSize, PAGE_EXECUTE_READWRITE, &oldProtect))
				return FALSE;

			// Write the rel jump
			PJMP_REL pJmp = (PJMP_REL)pPatchTarget;
			pJmp->opcode = 0xE9;
			pJmp->operand = (UINT32)((LPBYTE)ct.pRelay - (pPatchTarget + sizeof(JMP_REL)));

			if (ct.patchAbove)
			{
				// If patch above we added the rel jmp in the patch above area
				// we need to add the short jump to this are now in the target function
				PJMP_REL_SHORT pShortJmp = (PJMP_REL_SHORT)ct.pTarget;
				pShortJmp->opcode = 0xEB;
				pShortJmp->operand = (UINT8)(0 - (sizeof(JMP_REL) + sizeof(JMP_REL_SHORT)));
			}

			VirtualProtect(pPatchTarget, patchSize, oldProtect, &oldProtect);
			// In case those instructions were saved in cache
			FlushInstructionCache(GetCurrentProcess(), pPatchTarget, patchSize);

			// This to call the original function
			*ppOriginal = ct.pTrampoline;

			return TRUE;
		}

	}

	return FALSE;
}

int main(int argc, char* argv[])
{

	printf("About to test MessageBoxA hooking\n");

	printf("Triger: ' MessageBoxA(NULL, 'Hi its a test', 'test', MB_OK) '\n");

	// Execute origin function
	MessageBoxA(NULL, "Hi its a test\n", "test", MB_OK);

	// Hooking MessageBoxA
	printf("Hooking MessageBoxA\n");
	InstallMessageBoxAHook();

	printf("Triger: ' MessageBoxA(NULL, 'Hi its a test', 'test', MB_OK) '\n");
	// Execute origin function
	MessageBoxA(NULL, "Hi its a test\n", "test", MB_OK);


	printf("About to test MessageBeep hooking\n");

	printf("Triger: ' MessageBeep(MB_ICONINFORMATION) '\n");

	MessageBeep(MB_ICONINFORMATION);

	printf("Hooking MessageBeep\n");

	InstallMessageBeepAHook();

	printf("Triger: ' MessageBeep(MB_ICONINFORMATION) '\n");

	MessageBeep(MB_ICONINFORMATION);
	printf("Finished without crash\n");
}