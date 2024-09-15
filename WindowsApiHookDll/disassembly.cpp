#include "pch.h"
#include "disassembly.h"

// Function to decode bytes from binary code into assembly instructions.
void GetInstruction(const unsigned char* code, Instruction& inst)
{
	uint8_t x = 0;
	uint8_t c = 0;
	uint8_t* p = (uint8_t*)code;
	uint8_t cflags = 0;
	uint8_t opcode = 0;
	uint8_t* t = table;
	uint8_t pref = 0;
	uint8_t m_mod = 0;
	uint8_t m_reg = 0;
	uint8_t m_rm = 0;
	uint8_t disp_size = 0;
	uint8_t op64 = 0;

	// Initialize the instruction object to zero.
	memset(&inst, 0, sizeof(Instruction));

	// Scan and process instruction prefixes to modify behavior of following bytes
	for (x = 16; x > 0; x--);
	{
		switch (c = *p++)
		{
			// Operand size override prefix switches operand size fr
			// Handle segment override prefixes,
			// which change the default segment for memory addressing.
		case PREFIX_SEGMENT_CS:
		case PREFIX_SEGMENT_SS:
		case PREFIX_SEGMENT_DS:
		case PREFIX_SEGMENT_ES:
		case PREFIX_SEGMENT_FS:
		case PREFIX_SEGMENT_GS:
			pref |= PRE_SEG;
			inst.p_seg = c;
			break;
			// Operand size override prefix
			// (from 32-bit default to 16-bit in 64-bit mode).
		case PREFIX_OPERAND_SIZE:
			inst.p_66 = c;
			pref |= PRE_66;
			break;

			// Address size override prefix
			// (switches from default 64-bit to 32-bit addressing).
		case PREFIX_ADDRESS_SIZE:
			inst.p_67 = c;
			pref |= PRE_67;
			break;

			// Lock prefix
			// (ensures exclusive access to shared resources).
		case PREFIX_LOCK:
			inst.p_lock = c;
			pref |= PRE_LOCK;
			break;

			// REP prefixes
			// for string operations,
			// (modifies operation count based on ECX/RDX).
		case PREFIX_REPX:
			inst.p_lock = c;
			pref |= PRE_F3;
			break;
		case PREFIX_REPNZ:
			inst.p_lock = c;
			pref |= PRE_F2;
			break;

		default:
			// Exit loop if a non-prefix byte is encountered.
			goto prefix_done;
		}

	}


prefix_done:
	// Finalize prefix processing by encoding them into the instruction flags.
	inst.flags = (uint32_t)pref << 23;

	// No prefixes were detected.
	if (pref == 0)
	{
		pref |= PRE_NONE;
	}

	// REX prefix processing
	// for 64-bit operand size and extended registers.
	// We check the upper 4 bits equals 0100 which is REX prefix
	//    7   6   5   4   3   2   1   0
	//  +---+---+---+---+---+---+---+---+
	//  | 0   1   0   0 | W | R | X | B |
	//  +---+---+---+---+---+---+---+---+
	if ((c & 0xf0) == 0x40)
	{
		// Add rex prefix flag
		inst.flags |= F_PREFIX_REX;
		inst.rex_b = c & 0x01; // Base register extension.
		inst.rex_x = (c & 0x02) >> 1; // Index register extension.
		inst.rex_r = (c & 0x04) >> 2; // Register field extension.
		inst.rex_w = (c & 0x08) >> 3; // 64-bit operand size extension.

		// If rex_w is on it means that
		// We want to use 64bit operand sizing
		// Which in case of immediate values will cause us
		// To read more bytes, i.e MOV reg, imm64
		if (inst.rex_w && ((*p & 0xf8) == 0xb8))
		{
			op64++;
		}

		// Move to the next byte after the REX prefix.
		c = *p++;
	}

	// Opcode determination,
	// including possible extension for two-byte opcodes.
	inst.opcode = c;
	if (inst.opcode == 0x0f)
	{
		c = *p++;
		inst.opcode2 = c;
		// Move to extended opcode table.
		t += DELTA_OPCODES;
	}
	// special MOV instructions that
	// use direct memory addressing
	else if (c >= 0xa0 && c <= 0xa3)
	{
		// Adjust for operand size and addressing mode.
		if (pref & PRE_67)
		{
			// Add flag for operand size override
			// because the operand is the address
			pref |= PRE_66;
		}
		else {
			// If address sizing is off there is no
			// need for operand sizing
			pref &= ~PRE_66;
		}
	}

	opcode = c;
	int t_op_group_index = t[opcode / 4];
	int t_op_index = t_op_group_index + (opcode % 4);
	cflags = t[t_op_index];

	// Handle unrecognized opcode error.
	if (cflags == C_ERROR)
	{
		inst.flags |= C_ERROR;
		inst;
	}

	if (cflags & C_GROUP)
	{
		// Fetch extended flags from the table that corresponds
		// to this group of opcodes.
		uint16_t flags_new;
		// Read 16 bits from the table offset by the lower 7 bits of cflags.
		flags_new = *(uint16_t*)(t + (cflags & 0x7f));
		// Update cflags with the lower 8 bits of the fetched flags.
		cflags = (uint8_t)flags_new;
	}

	// Extended opcode handling.
	if (inst.opcode2)
	{
		t = table + DELTA_PREFIXES;
	}
	// ModR/M byte decoding:
	// specifies registers and addressing modes.
	if (cflags & C_MODRM)
	{
		inst.flags |= F_MODRM;
		c = *p++;
		m_mod = c >> 6;
		m_rm = c & 7;
		m_reg = (c & 0x3f) >> 3;

		inst.modrm = c;
		inst.modrm_mod = m_mod;
		inst.modrm_rm = m_rm;
		inst.modrm_reg = m_reg;


		// Special opcode handling
		// for register-direct addressing.
		if (inst.opcode2)
		{
			switch (opcode)
			{
				// Opcode 0x20: AND – 
				// Logical AND between the destination (register/memory) and source (register).
			case 0x20:
				// Opcode 0x22: AND – 
				// Logical AND between the destination (register) and source (memory).
			case 0x22:
				// Opcode 0x21: AND – 
				// Logical AND between the destination (memory) and source (register).
			case 0x21:
				// Opcode 0x23: AND – 
				// Logical AND between the destination (register) and source (memory).
			case 0x23:
				m_mod = 3; // Direct register mode.
				break;
			}
		}

		// Decoding based on ModR/M byte specifics.
		c = *p++;
		// The m_reg field represents the register portion
		if (m_reg <= 1)
		{
			if (opcode == 0xf6)
			{
				// Opcode 0xF6: TEST, DIV, IDIV, NOT, NEG, MUL, IMUL, INC, or DEC
				// The functionality of 0xF6 depends on the ModR/M byte.
				// For TEST, an immediate 8-bit value is used.
				cflags |= C_IMM8;
			}
			else if (opcode == 0xf7)
			{
				// Opcode 0xF7: TEST, DIV, IDIV, NOT, NEG, MUL, IMUL, INC, or DEC with a 16/32-bit operand
				// Similar to 0xF6 but operates on wider operands (16 or 32-bit, or even 64-bit with REX.W prefix in 64-bit mode).
				cflags |= C_IMM_P66; // Adjust for operand size prefix.
			}
		}

		switch (m_mod)
		{
			// Case 0 (Direct Memory Address)
		case 0:
			if (pref & PRE_67)
			{
				if (m_rm == 6)
				{
					// 16-bit displacement (compatible with 32-bit mode).
					disp_size = 2;
				}
			}
			else
			{
				if (m_rm == 5)
				{
					// 32-bit displacement (default in 64-bit mode).
					disp_size = 4;
				}
			}
			break;
			// Case 1 (Memory Addressing with 8-bit Displacement)
		case 1:
			// 8-bit displacement.
			disp_size = 1;
			break;
			// Case 2 (Memory Addressing with 16-bit or 32-bit Displacement)
		case 2:
			disp_size = 2;
			// 16 or 32-bit displacement based on addressing mode.
			if (!(pref & PRE_67))
			{
				// Double displacement if not in 32-bit addressing mode.
				disp_size <<= 1;
			}
			break;
		}

		// m_mod != 3: This ensures that the instruction is not
		// in register-to-register mode. SIB addressing
		// is only used for memory accesses, not register-to-register
		// operations.
		// m_rm == 4: In the ModR/M byte,
		// the R/M field indicates which register
		// or memory addressing mode is being used.
		// If R/M = 4, it indicates that the instruction uses
		// an SIB byte (Scale-Index-Base).
		if (m_mod != 3 && m_rm == 4)
		{
			//  +-------- - +-------- - +-------- - +
			//	|   Scale   |    Index  |    Base   |
			//	|    2 bits |    3 bits |    3 bits |
			//	+-------- - +-------- - +-------- - +
			inst.flags |= F_SIB;
			p++;
			inst.sib = c;
			inst.sib_scale = c >> 6;
			inst.sib_index = (c & 0x3f) >> 3;
			inst.sib_base = c & 7;
			// this indicates that the instruction uses no base register,
			// and instead, a 32 - bit displacement follows the SIB byte.
			if (inst.sib_base == 5 && !(m_mod & 1))
			{
				// 32-bit displacement with no base register.
				disp_size = 4;
			}
		}

		// If we reach here we went through all bytes
		p--;
		// Check the disposition size in bytes
		switch (disp_size)
		{
		case 1:
			// 0100 0000
			inst.flags |= F_DISP8;
			inst.disp.disp8 = *p;
			break;
		case 2:
			// 1000 0000
			inst.flags |= F_DISP16;
			inst.disp.disp16 = *(uint16_t*)p;
			break;
		case 4:
			// 0001 0000 0000
			inst.flags |= F_DISP32;
			inst.disp.disp16 = *(uint32_t*)p;
			break;
		}
		p += disp_size;
	}

	// If instruction is effected by the 0x66 prefix
	if (cflags & C_IMM_P66)
	{
		if (cflags & C_REL32)
		{
			if (cflags & PRE_66)
			{
				// Instruction is a relative 32bit and 0x66 prefix
				// So we need to change the sizing to 16bit
				inst.flags |= F_IMM16 | F_RELATIVE;
				inst.imm.imm16 = *(uint16_t*)p;
				// 16bit = 2bytes , so advence the pointer by 2 (bytes)
				p += 2;
				goto disasm_done;
			}
			goto rel32_ok;
		}
		else if (op64)
		{
			// 64bit Operand
			inst.flags |= F_IMM64;
			inst.imm.imm64 = *(uint64_t*)p;
			// 64bit = 8 bytes
			p += 8;
		}
		else if (!(pref & PRE_66))
		{
			// No 0x66 prefix, means we use default sizing to operand
			inst.flags |= F_IMM32;
			inst.imm.imm32 = *(uint32_t*)p;
			p += 4;
		}
		else {
			// 0x66 prefix is on, we use 16bit operand in this case
			goto imm16_ok;
		}
	}

	if (cflags & C_IMM16)
	{
	imm16_ok:
		inst.flags |= F_IMM16;
		inst.imm.imm16 = *(uint16_t*)p;
		p += 2;
	}
	if (cflags & C_IMM8) {
		inst.flags |= F_IMM8;
		inst.imm.imm8 = *p++;
	}
	if (cflags & C_REL32) {
	rel32_ok:
		inst.flags |= F_IMM32 | F_RELATIVE;
		inst.imm.imm32 = *(uint32_t*)p;
		p += 4;
	}
	else if (cflags & C_REL8) {
		inst.flags |= F_IMM8 | F_RELATIVE;
		inst.imm.imm8 = *p++;
	}

disasm_done:
	inst.len = (uint8_t)(p - (uint8_t*)code);
	if (inst.len > 15)
	{
		inst.flags |= F_ERROR_LENGTH;
	}

}
