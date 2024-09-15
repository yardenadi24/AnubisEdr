#include "pch.h"
#include "trampoline.h"
#include "disassembly.h"

// Define constants for memory sizes and protection flags
#define MEMORY_SLOT_SIZE 64
#define TRAMPOLINE_MAX_SIZE (MEMORY_SLOT_SIZE - sizeof(JMP_ABS))

// Memory protection flags to verify if an address is executable
#define PAGE_EXECUTE_FLAGS (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)

// Define assembly instruction opcodes as constants for better readability
const BYTE OPCODE_NOP = 0x90;             // No Operation
const BYTE OPCODE_INT3 = 0xCC;            // Breakpoint Interrupt
const BYTE OPCODE_LONG_JUMP = 0xFF;       // Opcode for long jumps and calls
const BYTE OPCODE_CALL_NEAR = 0xE8;        // Near Call
const BYTE OPCODE_JMP_NEAR = 0xE9;         // Near Jump
const BYTE OPCODE_SHORT_JUMP = 0xEB;       // Short Jump
const BYTE OPCODE_JCC = 0x70;              // Conditional Jump

// Define masks and bits for ModR/M byte to parse instructions
const BYTE MODRM_RIP_RELATIVE_MASK = 0xC7; // Mask to check RIP-relative addressing
const BYTE MODRM_RM_BITS = 0x05;           // Bits indicating RIP-relative mode
const BYTE MODRM_REG_BITS = 0x04;          // Bits indicating specific register usage

// Define sizes for various jump and call instructions
const UINT SIZE_JMP_REL = sizeof(JMP_REL);
const UINT SIZE_JMP_ABS = sizeof(JMP_ABS);
const UINT SIZE_ABS_CALL = sizeof(CALL_ABS);
const UINT SIZE_ABS_JCC = sizeof(JCC_ABS);
const UINT SIZE_ABS_JMP = sizeof(JMP_ABS);

/**
 * @brief Checks if a given memory address is executable.
 *
 * This function queries the memory information of the provided address and
 * verifies if it is committed and has execute permissions.
 *
 * @param pAddress Pointer to the memory address to check.
 * @return TRUE if the address is executable, FALSE otherwise.
 */
BOOL IsExecutableAddress(LPVOID pAddress)
{
    MEMORY_BASIC_INFORMATION mi;
    // Retrieve memory information for the address
    VirtualQuery(pAddress, &mi, sizeof(mi));

    // Check if the memory is committed and has execute permissions
    return (mi.State == MEM_COMMIT && (mi.Protect & PAGE_EXECUTE_FLAGS));
}

/**
 * @brief Checks if a block of code is padding (e.g., NOPs or INT3s).
 *
 * Padding is often used to align code or reserve space for future instructions.
 * This function verifies if a sequence of bytes consists solely of padding instructions.
 *
 * @param pInst Pointer to the start of the instruction bytes.
 * @param size Number of bytes to check.
 * @return TRUE if all bytes are padding, FALSE otherwise.
 */
static BOOL IsCodePadding(LPBYTE pInst, UINT size)
{
    UINT i;
    // First byte should be a known padding opcode (NOP, INT3, or 0x00)
    if (pInst[0] != OPCODE_NOP && pInst[0] != 0x00 && pInst[0] != OPCODE_INT3)
        return FALSE;

    // All subsequent bytes should match the first padding byte
    for (i = 1; i < size; ++i)
    {
        if (pInst[i] != pInst[0])
            return FALSE;
    }
    return TRUE;
}

/**
 * @brief Initializes a CALL_ABS structure with appropriate opcodes and default values.
 *
 * This structure is used to create an absolute call instruction in the trampoline.
 *
 * @param call_abs Reference to the CALL_ABS structure to initialize.
 */
VOID InitAbsCall(CALL_ABS& call_abs)
{
    call_abs.opcode0 = OPCODE_LONG_JUMP;      // Opcode for a long jump
    call_abs.opcode1 = 0x15;                  // Specific opcode byte for absolute call
    call_abs.offset_to_address = 0x2;         // Offset where the address will be placed
    call_abs.opcode_rel_jmp = OPCODE_SHORT_JUMP; // Opcode for a short jump after the call
    call_abs.offset_for_jmp = 0x8;            // Offset for the jump
    call_abs.address = 0;                      // Initialize address to zero
}

/**
 * @brief Initializes a JMP_ABS structure with appropriate opcodes and default values.
 *
 * This structure is used to create an absolute jump instruction in the trampoline.
 *
 * @param jmp_abs Reference to the JMP_ABS structure to initialize.
 */
VOID InitAbsJmp(JMP_ABS& jmp_abs)
{
    jmp_abs.opcode0 = OPCODE_LONG_JUMP;  // Opcode for a long jump
    jmp_abs.opcode1 = 0x25;              // Specific opcode byte for absolute jump
    jmp_abs.dummy = 0x00000000;          // Placeholder for alignment or future use
    jmp_abs.address = 0;                  // Initialize address to zero
}

/**
 * @brief Initializes a JCC_ABS structure with appropriate opcodes and default values.
 *
 * This structure is used to create an absolute conditional jump instruction in the trampoline.
 *
 * @param jcc_abs Reference to the JCC_ABS structure to initialize.
 */
VOID InitAbsJcc(JCC_ABS& jcc_abs)
{
    jcc_abs.opcode = OPCODE_JCC;       // Opcode for conditional jump
    jcc_abs.dummy0 = 0x0E;             // Placeholder bytes for alignment or future use
    jcc_abs.dummy1 = 0xFF;
    jcc_abs.dummy2 = 0x25;
    jcc_abs.dummy3 = 0x00000000;
    jcc_abs.address = 0;                // Initialize address to zero
}

/**
 * @brief Creates a trampoline function to redirect execution flow.
 *
 * A trampoline function is used in hooking to redirect execution from the original
 * function to a custom handler and then optionally back to the original function.
 *
 * @param ct Pointer to the TRAMPOLINE structure containing necessary information.
 * @return TRUE if the trampoline was created successfully, FALSE otherwise.
 */
BOOL CreateTrampolineFunction(PTRAMPOLINE ct)
{
    // Initialize structures for absolute calls, jumps, and conditional jumps
    CALL_ABS abs_call;
    JMP_ABS abs_jmp;
    JCC_ABS abs_jcc;

    InitAbsCall(abs_call);   // Set up the absolute call structure
    InitAbsJmp(abs_jmp);     // Set up the absolute jump structure
    InitAbsJcc(abs_jcc);     // Set up the absolute conditional jump structure

    // Initialize position trackers
    UINT8 oldPos = 0;         // Current byte position in the target function
    UINT8 newPos = 0;         // Current byte position in the trampoline function
    BOOL finished = FALSE;    // Flag to indicate if trampoline creation is complete

    // Initialize trampoline control structure
    ct->patchAbove = FALSE;   // Indicates if patching is above the target
    ct->nIP = 0;              // Instruction pointer index

    // Start copying instructions from the target function to the trampoline
    do
    {
        UINT8 instBuffer[16];      // Buffer to store the current instruction bytes
        ULONG_PTR jmpDest = 0;     // Destination address for a jump, if applicable
        Instruction Inst;          // Structure to hold the decoded instruction
        UINT copySize;             // Number of bytes to copy for the current instruction
        LPVOID pCopySrc;           // Source pointer for copying the instruction

        // Calculate the address of the current instruction in the target function
        ULONG_PTR pOldInst = (ULONG_PTR)ct->pTarget + oldPos;
        // Calculate the address of the current position in the trampoline function
        ULONG_PTR pNewInst = (ULONG_PTR)ct->pTrampoline + newPos;

        // Decode the current instruction at pOldInst
        GetInstruction((unsigned char*)pOldInst, Inst);
        copySize = Inst.len;  // Determine the length of the current instruction

        // If there was an error decoding the instruction, abort trampoline creation
        if (Inst.flags & F_ERROR)
            return FALSE;

        // By default, set the source for copying as the current instruction in the target
        pCopySrc = (LPVOID)pOldInst;

        // **Condition 1:** Check if there's enough space to insert a JMP instruction
        if (oldPos >= sizeof(JMP_REL))
        {
            // If sufficient space is available, set up an absolute jump to redirect execution
            abs_jmp.address = pOldInst;    // Set the jump address to the current instruction
            pCopySrc = &abs_jmp;            // Use the absolute jump structure as the source
            copySize = sizeof(abs_jmp);     // Adjust the number of bytes to copy
            finished = TRUE;                // Indicate that trampoline creation is complete
        }
        // **Condition 2:** Check if the instruction uses RIP-relative addressing
        else if ((Inst.modrm & MODRM_RIP_RELATIVE_MASK) == MODRM_RM_BITS)
        {
            // RIP-relative addressing means the instruction's memory reference is relative to the instruction pointer
            PUINT32 pRelAddr;
            memcpy(instBuffer, (LPBYTE)pOldInst, copySize);  // Copy the instruction bytes to a buffer
            pCopySrc = instBuffer;                           // Set the copy source to the buffer

            // Determine the size of the immediate value based on instruction flags
            uint32_t imm_val_len_in_bytes = 0;
            if (Inst.flags & F_IMM8) imm_val_len_in_bytes = 1;
            if (Inst.flags & F_IMM16) imm_val_len_in_bytes = 2;
            if (Inst.flags & F_IMM32) imm_val_len_in_bytes = 4;
            if (Inst.flags & F_IMM64) imm_val_len_in_bytes = 8;

            // Calculate the relative address within the instruction
            pRelAddr = (PUINT32)(instBuffer + Inst.len - imm_val_len_in_bytes - 4);
            // Adjust the relative address to point correctly in the trampoline
            *pRelAddr = (UINT32)((pOldInst + Inst.len + (INT32)Inst.disp.disp32) - (pNewInst + Inst.len));

            // **Sub-Condition:** If the instruction is a long jump, mark as finished
            if (Inst.opcode == OPCODE_LONG_JUMP && Inst.modrm_reg == MODRM_REG_BITS)
                finished = TRUE;
        }
        // **Condition 3:** Check if the instruction is a near CALL
        else if (Inst.opcode == OPCODE_CALL_NEAR)
        {
            // Handle near CALL instructions by converting them to absolute CALLs in the trampoline
            ULONG_PTR dest = pOldInst + Inst.len + (INT32)Inst.imm.imm32; // Calculate the absolute destination
            abs_call.address = dest;     // Set the call destination address
            pCopySrc = &abs_call;        // Use the absolute call structure as the source
            copySize = sizeof(abs_call); // Adjust the number of bytes to copy
        }
        // **Condition 4:** Check if the instruction is a near JMP (short or long)
        else if ((Inst.opcode & 0xFD) == OPCODE_JMP_NEAR)
        {
            // Handle near JMP instructions by converting them to absolute JMPs in the trampoline
            ULONG_PTR dest = pOldInst + Inst.len; // Start with the address after the current instruction

            // Determine the jump destination based on whether it's a short or long jump
            if (Inst.opcode == OPCODE_SHORT_JUMP)
                dest += (INT8)Inst.imm.imm8;  // Short jump uses an 8-bit displacement
            else
                dest += (INT32)Inst.imm.imm32; // Long jump uses a 32-bit displacement

            // **Sub-Condition:** If the jump destination is within the target function's range
            if ((ULONG_PTR)ct->pTarget <= dest && dest < ((ULONG_PTR)ct->pTarget + sizeof(JMP_REL)))
            {
                // Track the farthest jump destination for safety
                if (jmpDest < dest)
                    jmpDest = dest;
            }
            else
            {
                // Convert the relative jump to an absolute jump
                abs_jmp.address = dest;        // Set the jump address to the destination
                pCopySrc = &abs_jmp;           // Use the absolute jump structure as the source
                copySize = sizeof(abs_jmp);    // Adjust the number of bytes to copy
                finished = pOldInst >= jmpDest; // Finish if we've processed all necessary jumps
            }
        }
        // **Condition 5:** Check for conditional jumps, loop instructions, or two-byte conditional jumps
        else if ((Inst.opcode & 0xF0) == OPCODE_JCC
            || (Inst.opcode & 0xFC) == 0xE0  // Handles LOOP/JCXZ instructions
            || (Inst.opcode2 & 0xF0) == 0x80) // Handles two-byte conditional jumps (e.g., 0x0F 0x8x)
        {
            // This block handles:
            // 1. **Conditional Jumps (JCC):** Instructions like JZ, JNZ, etc.
            // 2. **Loop Instructions:** Instructions like LOOP, JCXZ, etc.
            // 3. **Two-byte Conditional Jumps:** Extended conditional jumps requiring two opcode bytes

            ULONG_PTR dest = pOldInst + Inst.len; // Start with the address after the current instruction

            // Determine the jump destination based on the type of conditional jump
            if ((Inst.opcode & 0xF0) == OPCODE_JCC || (Inst.opcode & 0xFC) == 0xE0)
                dest += (INT8)Inst.imm.imm8;   // Short displacement (8-bit) for JCC and LOOP instructions
            else
                dest += (INT32)Inst.imm.imm32; // Long displacement (32-bit) for two-byte conditional jumps

            // **Sub-Condition:** If the jump destination is within the target function's range
            if ((ULONG_PTR)ct->pTarget <= dest && dest < ((ULONG_PTR)ct->pTarget + sizeof(JMP_REL)))
            {
                // Track the farthest jump destination for safety
                if (jmpDest < dest)
                    jmpDest = dest;
            }
            // **Sub-Condition:** If the instruction is a LOOP/JCXZ, we cannot modify it here
            else if ((Inst.opcode & 0xFC) == 0xE0)
            {
                // These instructions have specific behaviors that are not handled in this trampoline
                return FALSE; // Abort trampoline creation as it's unsupported
            }
            else
            {
                // For other conditional jumps, convert them to absolute conditional jumps
                UINT8 cond = ((Inst.opcode != 0x0F ? Inst.opcode : Inst.opcode2) & 0x0F); // Extract the condition code
                abs_jcc.opcode = 0x71 ^ cond;  // Invert the jump condition for correct behavior
                abs_jcc.address = dest;         // Set the conditional jump destination address
                pCopySrc = &abs_jcc;            // Use the absolute conditional jump structure as the source
                copySize = sizeof(abs_jcc);     // Adjust the number of bytes to copy
            }
        }
        // **Condition 6:** Check if the instruction is a RET (return) instruction
        else if ((Inst.opcode & 0xFE) == 0xC2)
        {
            // RET instructions signal the end of a function. Once encountered, we can finish trampoline creation.
            finished = (pOldInst >= jmpDest); // Ensure all necessary jumps have been handled before finishing
        }

        // **Safety Check:** Ensure that modifying the instruction length does not disrupt jump destinations
        if (pOldInst < jmpDest && copySize != Inst.len)
            return FALSE; // Abort if instruction length alteration affects jump targets

        // **Size Check:** Ensure the trampoline does not exceed its maximum allowed size
        if ((newPos + copySize) > TRAMPOLINE_MAX_SIZE)
            return FALSE; // Abort if trampoline becomes too large

        // **Instruction Pointer Check:** Ensure we haven't exceeded the maximum number of instructions
        if (ct->nIP >= ARRAYSIZE(ct->oldIPs))
            return FALSE; // Abort if too many instructions have been copied

        // Record the mapping of old and new instruction pointers for later reference
        ct->oldIPs[ct->nIP] = oldPos; // Record the original position in the target function
        ct->newIPs[ct->nIP] = newPos; // Record the corresponding position in the trampoline
        ct->nIP++;                      // Increment the instruction pointer index

        // **Copy the Instruction:**
        // Copy the determined number of bytes from the source to the trampoline
        memcpy((LPBYTE)ct->pTrampoline + newPos, pCopySrc, copySize);

        // Update position trackers for the next iteration
        newPos += copySize; // Move forward in the trampoline
        oldPos += Inst.len;  // Move forward in the target function

    } while (!finished); // Continue until we've finished copying necessary instructions

    // **Post-Copy Check:** Ensure that we've successfully copied all required instructions
    // If not, the trampoline creation has failed
    if (!finished)
        return FALSE;


    // Check if we dont have enough space for long jump
    if (oldPos < sizeof(JMP_REL) &&
        !IsCodePadding((LPBYTE)ct->pTarget + oldPos, sizeof(JMP_REL) - oldPos))
    {
        // Not enough space for long jump (include padding)
        // Try short jump to patch above area

        // Is there enough space for a short jump?
        if (oldPos < sizeof(JMP_REL_SHORT)
            && !IsCodePadding((LPBYTE)ct->pTarget + oldPos, sizeof(JMP_REL_SHORT) - oldPos))
        {
            // If not we simply cant perform a jump to the relay function.
            return FALSE;
        }

        // Can we place the long jump above the function?
        // If not, then there wont be any option to jump to the relay function
        if (!IsExecutableAddress((LPBYTE)ct->pTarget - sizeof(JMP_REL)) ||
            !IsCodePadding((LPBYTE)ct->pTarget - sizeof(JMP_REL), sizeof(JMP_REL)))
            return FALSE;

        ct->patchAbove = TRUE;
    }

    // Create a relay function.
    abs_jmp.address = (ULONG_PTR)ct->pDetour; // Set the jump address to jump to the Detour function
    // The relay "function" will start at the end of
    // The current trampoline instructions isnerted
    ct->pRelay = (LPBYTE)ct->pTrampoline + newPos;
    // Write the relay "function"
    memcpy(ct->pRelay, &abs_jmp, sizeof(abs_jmp));
    // **Success:** Trampoline was created successfully
    return TRUE;
}
