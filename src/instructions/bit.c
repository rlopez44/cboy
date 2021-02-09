// Implementation of the bit shift instructions

#include <stdint.h>
#include "cboy/instructions.h"
#include "cboy/gameboy.h"
#include "execute.h"

/* the Rotate Left Circular Accumulator instruction
 * ================================================
 * Rotate the A register to the left by one position, with
 * bit 7 moved to bit 0 and also stored into the carry flag.
 *
 * Flags affected
 * --------------
 * Zero Flag:          reset
 * Subtract Flag:      reset
 * Half Carry Flag:    reset
 * Carry Flag:         set according to result
 */
void rlca(gameboy *gb)
{
    uint8_t bit_seven = gb->cpu->reg->a >> 7;
    gb->cpu->reg->a = (gb->cpu->reg->a << 1) | bit_seven;
    set_flags(gb->cpu->reg, 0, 0, 0, bit_seven);
}

/* the Rotate Left Accumulator instruction
 * =======================================
 * Rotate the A register to the left by one position, with
 * the carry flag moved into bit 0 and bit 7 moved into
 * the carry flag.
 *
 * Flags affected
 * --------------
 * Zero Flag:          reset
 * Subtract Flag:      reset
 * Half Carry Flag:    reset
 * Carry Flag:         set according to result
 */
void rla(gameboy *gb)
{
    uint8_t bit_seven = gb->cpu->reg->a >> 7;
    gb->cpu->reg->a = (gb->cpu->reg->a << 1) | read_carry_flag(gb->cpu->reg);
    set_flags(gb->cpu->reg, 0, 0, 0, bit_seven);
}

/* the Rotate Right Circular Accumulator instruction
 * =================================================
 * Rotate the A register to the right by one position, with
 * bit 0 moved to bit 7 and also stored into the carry flag.
 *
 * Flags affected
 * --------------
 * Zero Flag:          reset
 * Subtract Flag:      reset
 * Half Carry Flag:    reset
 * Carry Flag:         set according to result
 */
void rrca(gameboy *gb)
{
    uint8_t bit_zero = gb->cpu->reg->a & 1;
    gb->cpu->reg->a = (bit_zero << 7) | (gb->cpu->reg->a >> 1);
    set_flags(gb->cpu->reg, 0, 0, 0, bit_zero);
}

/* the Rotate Right Accumulator instruction
 * ========================================
 * Rotate the A register to the right by one position, with
 * the carry flag moved into bit 7 and bit 0 moved into
 * the carry flag.
 *
 * Flags affected
 * --------------
 * Zero Flag:          reset
 * Subtract Flag:      reset
 * Half Carry Flag:    reset
 * Carry Flag:         set according to result
 */
void rra(gameboy *gb)
{
    uint8_t bit_zero = gb->cpu->reg->a & 1;
    gb->cpu->reg->a = (read_carry_flag(gb->cpu->reg) << 7) | (gb->cpu->reg->a >> 1);
    set_flags(gb->cpu->reg, 0, 0, 0, bit_zero);
}

/* the Rotate Left Circular instruction
 * ====================================
 *
 * RLC r8
 * ------
 * Rotate the given register to the left by one position, with
 * bit 7 moved to bit 0 and also stored into the carry flag.
 *
 * RLC [HL]
 * --------
 * Rotate the byte pointed to by HL to the left by one position,
 * with bit 7 moved to bit 0 and also stored into the carry flag.
 *
 * Flags affected
 * --------------
 * Zero Flag:          set if result is zero
 * Subtract Flag:      reset
 * Half Carry Flag:    reset
 * Carry Flag:         set according to result
 */
void rlc(gameboy *gb, gb_instruction *inst)
{
    uint8_t bit_seven;

    // handle PTR_HL separately, since we need to read from memory
    if (inst->op1 == PTR_HL)
    {
        // value to rotate from memory
        uint16_t addr = read_hl(gb->cpu->reg);
        uint8_t val = read_byte(gb->memory, addr);

        // rotate and set flags
        bit_seven = val >> 7;
        write_byte(gb->memory, addr, (val << 1) | bit_seven);
        set_flags(gb->cpu->reg, (val << 1) | bit_seven == 0, 0, 0, bit_seven);
    }
    else
    {
        uint8_t *reg; // the register to rotate
        switch (inst->op1)
        {
            case REG_A:
                reg = &(gb->cpu->reg->a);
                break;

            case REG_B:
                reg = &(gb->cpu->reg->b);
                break;

            case REG_C:
                reg = &(gb->cpu->reg->c);
                break;

            case REG_D:
                reg = &(gb->cpu->reg->d);
                break;

            case REG_E:
                reg = &(gb->cpu->reg->e);
                break;

            case REG_H:
                reg = &(gb->cpu->reg->h);
                break;

            case REG_L:
                reg = &(gb->cpu->reg->l);
                break;
        }

        // perform the rotation and set flags
        bit_seven = *reg >> 7;
        *reg = (*reg << 1) | bit_seven;
        set_flags(gb->cpu->reg, *reg == 0, 0, 0, bit_seven);
    }
}

/* the Rotate Right Circular instruction
 * =====================================
 *
 * RRC r8
 * ------
 * Rotate the given register to the right by one position, with
 * bit 0 moved to bit 7 and also stored into the carry flag.
 *
 * RRC [HL]
 * --------
 * Rotate the byte pointed to by HL to the right by one position,
 * with bit 0 moved to bit 7 and also stored into the carry flag.
 *
 * Flags affected
 * --------------
 * Zero Flag:          set if result is zero
 * Subtract Flag:      reset
 * Half Carry Flag:    reset
 * Carry Flag:         set according to result
 */
void rrc(gameboy *gb, gb_instruction *inst)
{
    uint8_t bit_zero;

    // handle PTR_HL separately, since we need to read from memory
    if (inst->op1 == PTR_HL)
    {
        // value to rotate from memory
        uint16_t addr = read_hl(gb->cpu->reg);
        uint8_t val = read_byte(gb->memory, addr);

        // rotate and set flags
        bit_zero = val & 1;
        write_byte(gb->memory, addr, (bit_zero << 7) | (val >> 1));
        set_flags(gb->cpu->reg, (bit_zero << 7) | (val >> 1), 0, 0, bit_zero);
    }
    else
    {
        uint8_t *reg; // the register to rotate
        switch (inst->op1)
        {
            case REG_A:
                reg = &(gb->cpu->reg->a);
                break;

            case REG_B:
                reg = &(gb->cpu->reg->b);
                break;

            case REG_C:
                reg = &(gb->cpu->reg->c);
                break;

            case REG_D:
                reg = &(gb->cpu->reg->d);
                break;

            case REG_E:
                reg = &(gb->cpu->reg->e);
                break;

            case REG_H:
                reg = &(gb->cpu->reg->h);
                break;

            case REG_L:
                reg = &(gb->cpu->reg->l);
                break;
        }

        // perform the rotation and set flags
        bit_zero = *reg & 1;
        *reg = (bit_zero << 7) | (*reg >> 1);
        set_flags(gb->cpu->reg, *reg == 0, 0, 0, bit_zero);
    }
}

/* the Rotate Left instruction
 * ===========================
 *
 * RL r8
 * -----
 * Rotate the given register to the left by one position, with
 * bit 7 moved into the carry flag and the carry flag moved
 * into bit 0.
 *
 * RL [HL]
 * -------
 * Rotate the byte pointed to by HL to the left by one position,
 * with bit 7 moved into the carry flag and the carry flag moved
 * into bit 0.
 *
 * Flags affected
 * --------------
 * Zero Flag:          set if result is zero
 * Subtract Flag:      reset
 * Half Carry Flag:    reset
 * Carry Flag:         set according to result
 */
void rl(gameboy *gb, gb_instruction *inst)
{
    uint8_t bit_seven,
            carry = read_carry_flag(gb->cpu->reg);

    // handle PTR_HL separately, since we need to read from memory
    if (inst->op1 == PTR_HL)
    {
        // value to rotate from memory
        uint16_t addr = read_hl(gb->cpu->reg);
        uint8_t val = read_byte(gb->memory, addr);

        // rotate and set flags
        bit_seven = val >> 7;
        write_byte(gb->memory, addr, (val << 1) | carry);
        set_flags(gb->cpu->reg, (val << 1) | carry == 0, 0, 0, bit_seven);
    }
    else
    {
        uint8_t *reg; // the register to rotate
        switch (inst->op1)
        {
            case REG_A:
                reg = &(gb->cpu->reg->a);
                break;

            case REG_B:
                reg = &(gb->cpu->reg->b);
                break;

            case REG_C:
                reg = &(gb->cpu->reg->c);
                break;

            case REG_D:
                reg = &(gb->cpu->reg->d);
                break;

            case REG_E:
                reg = &(gb->cpu->reg->e);
                break;

            case REG_H:
                reg = &(gb->cpu->reg->h);
                break;

            case REG_L:
                reg = &(gb->cpu->reg->l);
                break;
        }

        // perform the rotation and set flags
        bit_seven = *reg >> 7;
        *reg = (*reg << 1) | carry;
        set_flags(gb->cpu->reg, *reg == 0, 0, 0, bit_seven);
    }
}

/* the Rotate Right instruction
 * ============================
 *
 * RR r8
 * -----
 * Rotate the given register to the right by one position, with
 * bit 0 moved into the carry flag and the carry flag moved into
 * bit 7.
 *
 * RR [HL]
 * -------
 * Rotate the byte pointed to by HL to the right by one position,
 * with bit 0 moved into the carry flag and the carry flag moved
 * into bit 7.
 *
 * Flags affected
 * --------------
 * Zero Flag:          set if result is zero
 * Subtract Flag:      reset
 * Half Carry Flag:    reset
 * Carry Flag:         set according to result
 */
void rr(gameboy *gb, gb_instruction *inst)
{
    uint8_t bit_zero,
            carry = read_carry_flag(gb->cpu->reg);

    // handle PTR_HL separately, since we need to read from memory
    if (inst->op1 == PTR_HL)
    {
        // value to rotate from memory
        uint16_t addr = read_hl(gb->cpu->reg);
        uint8_t val = read_byte(gb->memory, addr);

        // rotate and set flags
        bit_zero = val & 1;
        write_byte(gb->memory, addr, (carry << 7) | (val >> 1));
        set_flags(gb->cpu->reg, (carry << 7) | (val >> 1), 0, 0, bit_zero);
    }
    else
    {
        uint8_t *reg; // the register to rotate
        switch (inst->op1)
        {
            case REG_A:
                reg = &(gb->cpu->reg->a);
                break;

            case REG_B:
                reg = &(gb->cpu->reg->b);
                break;

            case REG_C:
                reg = &(gb->cpu->reg->c);
                break;

            case REG_D:
                reg = &(gb->cpu->reg->d);
                break;

            case REG_E:
                reg = &(gb->cpu->reg->e);
                break;

            case REG_H:
                reg = &(gb->cpu->reg->h);
                break;

            case REG_L:
                reg = &(gb->cpu->reg->l);
                break;
        }

        // perform the rotation and set flags
        bit_zero = *reg & 1;
        *reg = (carry << 7) | (*reg >> 1);
        set_flags(gb->cpu->reg, *reg == 0, 0, 0, bit_zero);
    }
}

/* the Shift Left Arithmetic instruction
 * =====================================
 *
 * SLA r8
 * ------
 * Shift the given register to the left by one position, with
 * bit 7 moved to the carry flag and bit 0 reset.
 *
 * SLA [HL]
 * --------
 * Shift the byte pointed to by HL to the left by one position,
 * with bit 7 moved to the carry flag and bit 0 reset.
 *
 * Flags affected
 * --------------
 * Zero Flag:          set if result is zero
 * Subtract Flag:      reset
 * Half Carry Flag:    reset
 * Carry Flag:         set according to result
 */
void sla(gameboy *gb, gb_instruction *inst)
{
    uint8_t bit_seven;

    // handle PTR_HL separately, since we need to read from memory
    if (inst->op1 == PTR_HL)
    {
        // value to shift from memory
        uint16_t addr = read_hl(gb->cpu->reg);
        uint8_t val = read_byte(gb->memory, addr);

        // Shift and set flags. Note that bit zero is reset by the left shift
        bit_seven = val >> 7;
        write_byte(gb->memory, addr, val << 1);
        set_flags(gb->cpu->reg, (val << 1) == 0, 0, 0, bit_seven);
    }
    else
    {
        uint8_t *reg; // the register to shift
        switch (inst->op1)
        {
            case REG_A:
                reg = &(gb->cpu->reg->a);
                break;

            case REG_B:
                reg = &(gb->cpu->reg->b);
                break;

            case REG_C:
                reg = &(gb->cpu->reg->c);
                break;

            case REG_D:
                reg = &(gb->cpu->reg->d);
                break;

            case REG_E:
                reg = &(gb->cpu->reg->e);
                break;

            case REG_H:
                reg = &(gb->cpu->reg->h);
                break;

            case REG_L:
                reg = &(gb->cpu->reg->l);
                break;
        }

        // Perform the shift and set flags.
        // Note that bit zero is reset by the left shift
        bit_seven = *reg >> 7;
        *reg <<= 1;
        set_flags(gb->cpu->reg, *reg == 0, 0, 0, bit_seven);
    }
}

/* the Shift Right Arithmetic instruction
 * ======================================
 *
 * SRA r8
 * ------
 * Shift the given register to the right by one position, with bit 0
 * moved to the carry flag and bit 7 retaining its original value.
 *
 * SRA [HL]
 * --------
 * Shift the byte pointed to by HL to the right by one position,
 * with bit 0 moved to the carry flag and bit 7 retaining its
 * original value.
 *
 * Flags affected
 * --------------
 * Zero Flag:          set if result is zero
 * Subtract Flag:      reset
 * Half Carry Flag:    reset
 * Carry Flag:         set according to result
 */
void sra(gameboy *gb, gb_instruction *inst)
{
    uint8_t bit_zero;

    // handle PTR_HL separately, since we need to read from memory
    if (inst->op1 == PTR_HL)
    {
        // value to shift from memory
        uint16_t addr = read_hl(gb->cpu->reg);
        uint8_t val = read_byte(gb->memory, addr);

        // Shift and set flags.
        bit_zero = val & 1;
        write_byte(gb->memory, addr, (val & 0x80) | (val >> 1));
        set_flags(gb->cpu->reg, (val & 0x80) | (val >> 1) == 0, 0, 0, bit_zero);
    }
    else
    {
        uint8_t *reg; // the register to shift
        switch (inst->op1)
        {
            case REG_A:
                reg = &(gb->cpu->reg->a);
                break;

            case REG_B:
                reg = &(gb->cpu->reg->b);
                break;

            case REG_C:
                reg = &(gb->cpu->reg->c);
                break;

            case REG_D:
                reg = &(gb->cpu->reg->d);
                break;

            case REG_E:
                reg = &(gb->cpu->reg->e);
                break;

            case REG_H:
                reg = &(gb->cpu->reg->h);
                break;

            case REG_L:
                reg = &(gb->cpu->reg->l);
                break;
        }

        // Perform the shift and set flags.
        bit_zero = *reg & 1;
        *reg = (*reg & 0x80) | (*reg >> 1);
        set_flags(gb->cpu->reg, *reg == 0, 0, 0, bit_zero);
    }
}

/* the Shift Right Logic instruction
 * =================================
 *
 * SRL r8
 * ------
 * Shift the given register to the right by one position,
 * with bit 0 moved to the carry flag and bit 7 reset.
 *
 * SRL [HL]
 * --------
 * Shift the byte pointed to by HL to the right by one position,
 * with bit 0 moved to the carry flag and bit 7 reset.
 *
 * Flags affected
 * --------------
 * Zero Flag:          set if result is zero
 * Subtract Flag:      reset
 * Half Carry Flag:    reset
 * Carry Flag:         set according to result
 */
void srl(gameboy *gb, gb_instruction *inst)
{
    uint8_t bit_zero;

    // handle PTR_HL separately, since we need to read from memory
    if (inst->op1 == PTR_HL)
    {
        // value to shift from memory
        uint16_t addr = read_hl(gb->cpu->reg);
        uint8_t val = read_byte(gb->memory, addr);

        // Shift and set flags. Bit seven reset by the shift
        bit_zero = val & 1;
        write_byte(gb->memory, addr, val >> 1);
        set_flags(gb->cpu->reg, val >> 1 == 0, 0, 0, bit_zero);
    }
    else
    {
        uint8_t *reg; // the register to shift
        switch (inst->op1)
        {
            case REG_A:
                reg = &(gb->cpu->reg->a);
                break;

            case REG_B:
                reg = &(gb->cpu->reg->b);
                break;

            case REG_C:
                reg = &(gb->cpu->reg->c);
                break;

            case REG_D:
                reg = &(gb->cpu->reg->d);
                break;

            case REG_E:
                reg = &(gb->cpu->reg->e);
                break;

            case REG_H:
                reg = &(gb->cpu->reg->h);
                break;

            case REG_L:
                reg = &(gb->cpu->reg->l);
                break;
        }

        // Perform the shift and set flags.
        // Bit seven is reset by the shift.
        bit_zero = *reg & 1;
        *reg >>= 1;
        set_flags(gb->cpu->reg, *reg == 0, 0, 0, bit_zero);
    }
}

/* The SWAP instruction
 * ====================
 *
 * SWAP r8
 * -------
 * Swap the upper nibble in the given
 * register with the lower nibble.
 *
 * SWAP [HL]
 * ---------
 * Swap the upper nibble in the byte
 * pointed to by HL with the lower nibble.
 *
 * Flags affected
 * --------------
 * Zero Flag:          set if result is zero
 * Subtract Flag:      reset
 * Half Carry Flag:    reset
 * Carry Flag:         reset
 */
void swap(gameboy *gb, gb_instruction *inst)
{
    // handle PTR_HL separately, since we need to read from memory
    if (inst->op1 == PTR_HL)
    {
        // value to swap from memory
        uint16_t addr = read_hl(gb->cpu->reg);
        uint8_t val = read_byte(gb->memory, addr);

        // swap nibbles
        uint8_t result = (val << 4) | (val >> 4);
        write_byte(gb->memory, addr, result);
        set_flags(gb->cpu->reg, result == 0, 0, 0, 0);
    }
    else
    {
        uint8_t *reg; // the register to swap
        switch (inst->op1)
        {
            case REG_A:
                reg = &(gb->cpu->reg->a);
                break;

            case REG_B:
                reg = &(gb->cpu->reg->b);
                break;

            case REG_C:
                reg = &(gb->cpu->reg->c);
                break;

            case REG_D:
                reg = &(gb->cpu->reg->d);
                break;

            case REG_E:
                reg = &(gb->cpu->reg->e);
                break;

            case REG_H:
                reg = &(gb->cpu->reg->h);
                break;

            case REG_L:
                reg = &(gb->cpu->reg->l);
                break;
        }

        // swap nibbles
        *reg = (*reg << 4) | (*reg >> 4);
        set_flags(gb->cpu->reg, *reg == 0, 0, 0, 0);
    }
}
