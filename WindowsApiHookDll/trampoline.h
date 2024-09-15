#pragma once
#include <Windows.h>

// Compile without padding
#pragma pack(push,1)

// A short(8 - bit) relative jump
// means the processor will jump to a location within a range
// of - 128 to + 127 bytes 
// relative to the current instruction pointer(RIP).
// Example: EB 05 ; JMP +0x05
typedef struct _JMP_REL_SHORT
{
	UINT8 opcode;
	UINT8 operand;
}JMP_REL_SHORT, * PJMP_REL_SHORT;

// Jump(E9) : This is a 32 - bit relative jump
// that moves RIP forward or backward by the specified number
// of bytes The address is calculated
// relative to the RIP.
typedef struct _JMP_REL
{
	UINT8 opcode;
	UINT32 operand;
}JMP_REL, * PJMP_REL;

// Call (E8): This is a 32-bit relative call to a function,
// meaning the processor saves the current instruction pointer to the stack
// (for a return later) and jumps to the target address,
// which is calculated relative to the current RIP.
typedef struct _CALL_REL
{
	UINT8 opcode;
	UINT32 operand;
}CALL_REL, * PCALL_REL;

// This instruction performs an indirect absolute jump
// to a 64-bit address stored in memory.
// The address is not specified directly in the instruction itself
// but is located at the memory address that is relative
// to the RIP. After this instruction,
// the CPU loads the absolute address
// from memory and jumps to that location.
typedef struct _JMP_ABS
{
	UINT8 opcode0;
	UINT8 opcode1;
	UINT32 dummy;
	UINT64 address;
}JMP_ABS, * PJMP_ABS;

// This instruction performs an indirect absolute call
// to a function located at a 64-bit address in memory.
// Similar to the indirect absolute jump,
// the absolute address is stored in memory and
// is loaded relative to the current RIP.
// The CPU pushes the current RIP onto the stack (for returning)
// and then jumps to the target address.
// In this struct we add Jmp right after the call
// to jmp over the address
typedef struct _CALL_ABS
{
	UINT8  opcode0;				// FF
	UINT8  opcode1;				// 15 
	UINT32 offset_to_address;   // Should be 0x02
	UINT8  opcode_rel_jmp;      // EB
	UINT8  offset_for_jmp;		// should be 0x08
	UINT64 address;				// Absolute destination address
} CALL_ABS, * PCALL_ABS;

// 32-bit direct relative conditional jumps.
typedef struct _JCC_REL
{
	UINT8  opcode0;     // 0F8* 
	UINT8  opcode1;
	UINT32 operand;     // Relative destination address
} JCC_REL;

// 64-bit indirect absolute conditional jumps that x64 lacks.
typedef struct _JCC_ABS
{
	UINT8  opcode;      // 7* 
	UINT8  dummy0;		// 0x0E
	UINT8  dummy1;      // FF25 00000000: JMP [+6]
	UINT8  dummy2;
	UINT32 dummy3;
	UINT64 address;     // Absolute destination address
} JCC_ABS;

#pragma pack(pop)

typedef struct _TRAMPOLINE
{
	LPVOID pTarget;         // [IN] Address of the target function.
	LPVOID pDetour;         // [IN] Address of the detour function.
	LPVOID pTrampoline;     // [IN] Buffer address for the trampoline and relay function.
	LPVOID pRelay;          // [OUT] Address of the relay function.
	BOOL   patchAbove;      // [OUT] Should use the hot patch area?
	UINT   nIP;             // [OUT] Number of the instruction boundaries.
	UINT8  oldIPs[8];       // [OUT] Instruction boundaries of the target function.
	UINT8  newIPs[8];       // [OUT] Instruction boundaries of the trampoline function.
} TRAMPOLINE, * PTRAMPOLINE;

BOOL CreateTrampolineFunction(PTRAMPOLINE ct);