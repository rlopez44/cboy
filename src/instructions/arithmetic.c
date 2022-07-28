// Implementation of the arithmetic instruction sets

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "cboy/instructions.h"
#include "cboy/gameboy.h"
#include "cboy/cpu.h"
#include "cboy/memory.h"
#include "cboy/log.h"
#include "execute.h"

// the increment instruction
void inc(gameboy *gb, gb_instruction *inst)
{
    /* NOTE: Flags are affected by the INC r8 and INC [HL] instructions.
     * Zero Flag:         set if result is 0
     * Subtract Flag:     reset
     * Half Carry Flag:   set if overflow from bit 3
     */
    switch (inst->op1)
    {
        case REG_A:
        {
            // store old value for half carry flag check
            uint8_t old_a = (gb->cpu->reg->a)++;

            set_zero_flag(gb->cpu->reg, gb->cpu->reg->a == 0);
            set_subtract_flag(gb->cpu->reg, 0);
            set_half_carry_flag(gb->cpu->reg, (old_a & 0xf) + 1 > 0xf);
            break;
        }

        case REG_B:
        {
            uint8_t old_b = (gb->cpu->reg->b)++;

            set_zero_flag(gb->cpu->reg, gb->cpu->reg->b == 0);
            set_subtract_flag(gb->cpu->reg, 0);
            set_half_carry_flag(gb->cpu->reg, (old_b & 0xf) + 1 > 0xf);
            break;
        }

        case REG_C:
        {
            uint8_t old_c = (gb->cpu->reg->c)++;

            set_zero_flag(gb->cpu->reg, gb->cpu->reg->c == 0);
            set_subtract_flag(gb->cpu->reg, 0);
            set_half_carry_flag(gb->cpu->reg, (old_c & 0xf) + 1 > 0xf);
            break;
        }

        case REG_D:
        {
            uint8_t old_d = (gb->cpu->reg->d)++;

            set_zero_flag(gb->cpu->reg, gb->cpu->reg->d == 0);
            set_subtract_flag(gb->cpu->reg, 0);
            set_half_carry_flag(gb->cpu->reg, (old_d & 0xf) + 1 > 0xf);
            break;
        }

        case REG_E:
        {
            uint8_t old_e = (gb->cpu->reg->e)++;

            set_zero_flag(gb->cpu->reg, gb->cpu->reg->e == 0);
            set_subtract_flag(gb->cpu->reg, 0);
            set_half_carry_flag(gb->cpu->reg, (old_e & 0xf) + 1 > 0xf);
            break;
        }

        case REG_H:
        {
            uint8_t old_h = (gb->cpu->reg->h)++;

            set_zero_flag(gb->cpu->reg, gb->cpu->reg->h == 0);
            set_subtract_flag(gb->cpu->reg, 0);
            set_half_carry_flag(gb->cpu->reg, (old_h & 0xf) + 1 > 0xf);
            break;
        }

        case REG_L:
        {
            uint8_t old_l = (gb->cpu->reg->l)++;

            set_zero_flag(gb->cpu->reg, gb->cpu->reg->l == 0);
            set_subtract_flag(gb->cpu->reg, 0);
            set_half_carry_flag(gb->cpu->reg, (old_l & 0xf) + 1 > 0xf);
            break;
        }

        case PTR_HL:
        {
            uint16_t addr = read_hl(gb->cpu->reg);
            uint8_t old_val = read_byte(gb, addr);
            write_byte(gb, addr, old_val + 1);

            set_zero_flag(gb->cpu->reg, old_val + 1 == 0);
            set_subtract_flag(gb->cpu->reg, 0);
            set_half_carry_flag(gb->cpu->reg, (old_val & 0xf) + 1 > 0xf);
            break;
        }

        case REG_BC:
            write_bc(gb->cpu->reg, read_bc(gb->cpu->reg) + 1);
            break;

        case REG_DE:
            write_de(gb->cpu->reg, read_de(gb->cpu->reg) + 1);
            break;

        case REG_HL:
            write_hl(gb->cpu->reg, read_hl(gb->cpu->reg) + 1);
            break;

        case REG_SP:
            ++(gb->cpu->reg->sp);
            break;

        default: // shouldn't get here
            LOG_ERROR("Illegal argument in %s encountered. Exiting...\n", inst->inst_str);
            exit(1);
    }

    LOG_DEBUG("%s %s\n", inst->inst_str, operand_strs[inst->op1]);
}

// the decrement instruction
void dec(gameboy *gb, gb_instruction *inst)
{
    /* NOTE: Flags are affected by the DEC r8 and DEC [HL] instructions.
     * Zero Flag:         set if result is 0
     * Subtract Flag:     set
     * Half Carry Flag:   set if borrow from bit 4
     */
    switch (inst->op1)
    {
        case REG_A:
        {
            // store old value for half-carry flag check
            uint8_t old_a = (gb->cpu->reg->a)--;

            set_zero_flag(gb->cpu->reg, gb->cpu->reg->a == 0);
            set_subtract_flag(gb->cpu->reg, 1);
            // borrow from bit 4 occurs if lower nibble is < 1 (i.e. 0)
            set_half_carry_flag(gb->cpu->reg, (old_a & 0xf) == 0);
            break;
        }

        case REG_B:
        {
            uint8_t old_b = (gb->cpu->reg->b)--;

            set_zero_flag(gb->cpu->reg, gb->cpu->reg->b == 0);
            set_subtract_flag(gb->cpu->reg, 1);
            set_half_carry_flag(gb->cpu->reg, (old_b & 0xf) == 0);
            break;
        }

        case REG_C:
        {
            uint8_t old_c = (gb->cpu->reg->c)--;

            set_zero_flag(gb->cpu->reg, gb->cpu->reg->c == 0);
            set_subtract_flag(gb->cpu->reg, 1);
            set_half_carry_flag(gb->cpu->reg, (old_c & 0xf) == 0);
            break;
        }

        case REG_D:
        {
            uint8_t old_d = (gb->cpu->reg->d)--;

            set_zero_flag(gb->cpu->reg, gb->cpu->reg->d == 0);
            set_subtract_flag(gb->cpu->reg, 1);
            set_half_carry_flag(gb->cpu->reg, (old_d & 0xf) == 0);
            break;
        }

        case REG_E:
        {
            uint8_t old_e = (gb->cpu->reg->e)--;

            set_zero_flag(gb->cpu->reg, gb->cpu->reg->e == 0);
            set_subtract_flag(gb->cpu->reg, 1);
            set_half_carry_flag(gb->cpu->reg, (old_e & 0xf) == 0);
            break;
        }

        case REG_H:
        {
            uint8_t old_h = (gb->cpu->reg->h)--;

            set_zero_flag(gb->cpu->reg, gb->cpu->reg->h == 0);
            set_subtract_flag(gb->cpu->reg, 1);
            set_half_carry_flag(gb->cpu->reg, (old_h & 0xf) == 0);
            break;
        }

        case REG_L:
        {
            uint8_t old_l = (gb->cpu->reg->l)--;

            set_zero_flag(gb->cpu->reg, gb->cpu->reg->l == 0);
            set_subtract_flag(gb->cpu->reg, 1);
            set_half_carry_flag(gb->cpu->reg, (old_l & 0xf) == 0);
            break;
        }

        case PTR_HL:
        {
            uint16_t addr = read_hl(gb->cpu->reg);
            uint8_t old_val = read_byte(gb, addr);
            write_byte(gb, addr, old_val - 1);

            set_zero_flag(gb->cpu->reg, old_val - 1 == 0);
            set_subtract_flag(gb->cpu->reg, 1);
            set_half_carry_flag(gb->cpu->reg, (old_val & 0xf) == 0);
            break;
        }

        case REG_BC:
            write_bc(gb->cpu->reg, read_bc(gb->cpu->reg) - 1);
            break;

        case REG_DE:
            write_de(gb->cpu->reg, read_de(gb->cpu->reg) - 1);
            break;

        case REG_HL:
            write_hl(gb->cpu->reg, read_hl(gb->cpu->reg) - 1);
            break;

        case REG_SP:
            --(gb->cpu->reg->sp);
            break;

        default: // shouldn't get here
            LOG_ERROR("Illegal argument in %s encountered. Exiting...\n", inst->inst_str);
            exit(1);
    }

    LOG_DEBUG("%s %s\n", inst->inst_str, operand_strs[inst->op1]);
}

// the add instruction
void add(gameboy *gb, gb_instruction *inst)
{
    /* NOTE: See below for affected flags.
     *
     * ADD A, r8 or ADD A, [HL] or ADD A, n8
     * -------------------------------------
     *  Zero Flag:         set if result is zero
     *  Subtract Flag:     reset
     *  Half Carry Flag:   set if overflow from bit 3
     *  Carry Flag:        set if overflow from bit 7
     *
     *  ADD HL, r16 (including SP)
     *  --------------------------
     *  Subtract Flag:     reset
     *  Half Carry Flag:   set if overflow from bit 11
     *  Carry Flag:        set if overflow from bit 15
     *
     *  ADD SP, e8 (signed 8-bit)
     *  -------------------------
     *  Zero Flag:         reset
     *  Subtract Flag:     reset
     *  Half Carry Flag:   set if overflow from bit 3
     *  Carry Flag:        set if overflow from bit 7
     */
    switch (inst->op1)
    {
        case REG_A:
        {
            uint8_t to_add = 0; // value to add
            switch (inst->op2)
            {
                case REG_A:
                    to_add = gb->cpu->reg->a;
                    break;

                case REG_B:
                    to_add = gb->cpu->reg->b;
                    break;

                case REG_C:
                    to_add = gb->cpu->reg->c;
                    break;

                case REG_D:
                    to_add = gb->cpu->reg->d;
                    break;

                case REG_E:
                    to_add = gb->cpu->reg->e;
                    break;

                case REG_H:
                    to_add = gb->cpu->reg->h;
                    break;

                case REG_L:
                    to_add = gb->cpu->reg->l;
                    break;

                case PTR_HL:
                    to_add = read_byte(gb, read_hl(gb->cpu->reg));
                    break;

                case IMM_8:
                    to_add = read_byte(gb, (gb->cpu->reg->pc)++);
                    break;

                default: // shouldn't get here
                    LOG_ERROR("Illegal argument in %s A, r8 encountered. Exiting...\n", inst->inst_str);
                    exit(1);
            }
            uint8_t old_a = gb->cpu->reg->a;
            gb->cpu->reg->a += to_add;
            set_flags(gb->cpu->reg,
                      gb->cpu->reg->a == 0,                       // zero
                      0,                                          // subtract
                      (old_a & 0xf) + (to_add & 0xf) > 0xf,       // half carry
                      (uint16_t)old_a + (uint16_t)to_add > 0xff); // carry
            break;
        }

        case REG_HL:
        {
            uint16_t to_add = 0; // value to add
            switch (inst->op2)
            {
                case REG_BC:
                    to_add = read_bc(gb->cpu->reg);
                    break;

                case REG_DE:
                    to_add = read_de(gb->cpu->reg);
                    break;

                case REG_HL:
                    to_add = read_hl(gb->cpu->reg);
                    break;

                case REG_SP:
                    to_add = gb->cpu->reg->sp;
                    break;

                default: // shouldn't get here
                    LOG_ERROR("Illegal argument in %s HL, r16 encountered. Exiting...\n", inst->inst_str);
                    exit(1);
            }
            uint16_t old_hl = read_hl(gb->cpu->reg);
            write_hl(gb->cpu->reg, old_hl + to_add);
            set_subtract_flag(gb->cpu->reg, 0);
            set_half_carry_flag(gb->cpu->reg, (old_hl & 0xfff) + (to_add & 0xfff) > 0xfff);
            set_carry_flag(gb->cpu->reg, (uint32_t)old_hl + (uint32_t)to_add > 0xffff);
            break;
        }

        case REG_SP: // single case, add signed 8-bit offset
        {
            // cast is safe since we don't change integer width
            int8_t offset = (int8_t)read_byte(gb, (gb->cpu->reg->pc)++);
            // explicit casts to avoid bugs due to implicit integer conversions
            gb->cpu->reg->sp = (int32_t)gb->cpu->reg->sp + (int32_t)offset;

            bool half_carry = 0, carry = 0;
            // overflow can occur only if offset > 0
            if (offset > 0)
            {
                half_carry = (gb->cpu->reg->sp & 0xf) + (offset & 0xf) > 0xf;
                carry = (gb->cpu->reg->sp & 0xff) + offset > 0xff;
            }
            set_flags(gb->cpu->reg, 0, 0, half_carry, carry);
            break;
        }

        default: // shouldn't get here
            LOG_ERROR("Illegal argument in %s encountered. Exiting...\n", inst->inst_str);
            exit(1);
    }

    LOG_DEBUG("%s %s, %s\n", inst->inst_str, operand_strs[inst->op1], operand_strs[inst->op2]);
}

// the add with carry (ADC) instruction
void adc(gameboy *gb, gb_instruction *inst)
{
    /* NOTE: first operand of ADC is always the A register
     *
     * Affected flags
     * --------------
     *  Zero Flag:         set if result is zero
     *  Subtract Flag:     reset
     *  Half Carry Flag:   set if overflow from bit 3
     *  Carry Flag:        set if overflow from bit 7
     */

    uint8_t to_add = read_carry_flag(gb->cpu->reg); // initialize to value of carry flag
    switch (inst->op2)
    {
        case REG_A:
            to_add += gb->cpu->reg->a;
            break;

        case REG_B:
            to_add += gb->cpu->reg->b;
            break;

        case REG_C:
            to_add += gb->cpu->reg->c;
            break;

        case REG_D:
            to_add += gb->cpu->reg->d;
            break;

        case REG_E:
            to_add += gb->cpu->reg->e;
            break;

        case REG_H:
            to_add += gb->cpu->reg->h;
            break;

        case REG_L:
            to_add += gb->cpu->reg->l;
            break;

        case PTR_HL:
            to_add += read_byte(gb, read_hl(gb->cpu->reg));
            break;

        case IMM_8:
            to_add += read_byte(gb, (gb->cpu->reg->pc)++);
            break;

        default: // shouldn't get here
            LOG_ERROR("Illegal argument in %s encountered. Exiting...\n", inst->inst_str);
            exit(1);
    }
    uint8_t old_a = gb->cpu->reg->a;
    gb->cpu->reg->a += to_add;
    set_flags(gb->cpu->reg,
              gb->cpu->reg->a == 0,                       // zero
              0,                                          // subtract
              (old_a & 0xf) + (to_add & 0xf) > 0xf,       // half carry
              (uint16_t)old_a + (uint16_t)to_add > 0xff); // carry

    LOG_DEBUG("%s %s, %s\n", inst->inst_str, operand_strs[inst->op1], operand_strs[inst->op2]);
}

/* Helper function used by the SUB, SBC, and CP instructions.
 * Calculates the difference between register A and to_sub,
 * stores the result in register A if needed (for SUB and SBC),
 * and sets flags accordingly.
 */
static void sub_from_reg_a(gb_registers *reg, uint8_t to_sub, bool store_result)
{
    /* NOTE: Regardless of which of the three instructions
     * calls this function, the four flags are set according
     * to the rules below given a value for to_sub.
     * -----------------------------------------------------
     *
     *  Zero Flag:         set if result is zero
     *  Subtract Flag:     set
     *  Half Carry Flag:   set if borrow from bit 4
     *  Carry Flag:        set if borrow (set if to_sub > A)
     */

    // save original value of register A
    uint8_t old_a = reg->a;

    // store the result if needed (SUB and SBC only)
    if (store_result)
    {
        reg->a -= to_sub;
    }

    // set flags accordingly
    set_flags(reg,
              old_a - to_sub == 0,            // zero
              1,                              // subtract
              (old_a & 0xf) < (to_sub & 0xf), // half carry
              old_a < to_sub);                // carry
}

// the subtract instruction
void sub(gameboy *gb, gb_instruction *inst)
{
    /* NOTE: First operand of SUB is always the A register
     *
     * Affected Flags
     * --------------
     *  Zero Flag:         set if result is zero
     *  Subtract Flag:     set
     *  Half Carry Flag:   set if borrow from bit 4
     *  Carry Flag:        set if borrow (set if op2 > A)
     *
     *  NOTE: SUB A, A will always set the zero flag
     */

    uint8_t to_sub = 0; // the value to subtract
    switch (inst->op2)
    {
        case REG_A:
            to_sub = gb->cpu->reg->a;
            break;

        case REG_B:
            to_sub = gb->cpu->reg->b;
            break;

        case REG_C:
            to_sub = gb->cpu->reg->c;
            break;

        case REG_D:
            to_sub = gb->cpu->reg->d;
            break;

        case REG_E:
            to_sub = gb->cpu->reg->e;
            break;

        case REG_H:
            to_sub = gb->cpu->reg->h;
            break;

        case REG_L:
            to_sub = gb->cpu->reg->l;
            break;

        case PTR_HL:
            to_sub = read_byte(gb, read_hl(gb->cpu->reg));
            break;

        case IMM_8:
            to_sub = read_byte(gb, (gb->cpu->reg->pc)++);
            break;

        default: // shouldn't get here
            LOG_ERROR("Illegal argument in %s encountered. Exiting...\n", inst->inst_str);
            exit(1);
    }

    // calculate and store the difference and set flags
    sub_from_reg_a(gb->cpu->reg, to_sub, 1);

    LOG_DEBUG("%s %s, %s\n", inst->inst_str, operand_strs[inst->op1], operand_strs[inst->op2]);
}

// the subtract with carry (SBC) instruction
void sbc(gameboy *gb, gb_instruction *inst)
{
    /* NOTE: First operand of SBC is always the A register
     *
     * Affected Flags
     * --------------
     * Zero Flag:         set if result is zero
     * Subtract Flag:     set
     * Half Carry Flag:   set if borrow from bit 4
     * Carry Flag:        set if borrow (set if op2 + carry > A)
     */

    uint8_t to_sub = read_carry_flag(gb->cpu->reg); // initialize to value of carry flag
    switch (inst->op2)
    {
        case REG_A:
            to_sub += gb->cpu->reg->a;
            break;

        case REG_B:
            to_sub += gb->cpu->reg->b;
            break;

        case REG_C:
            to_sub += gb->cpu->reg->c;
            break;

        case REG_D:
            to_sub += gb->cpu->reg->d;
            break;

        case REG_E:
            to_sub += gb->cpu->reg->e;
            break;

        case REG_H:
            to_sub += gb->cpu->reg->h;
            break;

        case REG_L:
            to_sub += gb->cpu->reg->l;
            break;

        case PTR_HL:
            to_sub += read_byte(gb, read_hl(gb->cpu->reg));
            break;

        case IMM_8:
            to_sub += read_byte(gb, (gb->cpu->reg->pc)++);
            break;

        default: // shouldn't get here
            LOG_ERROR("Illegal argument in %s encountered. Exiting...\n", inst->inst_str);
            exit(1);
    }

    // calculate and store the difference and set flags
    sub_from_reg_a(gb->cpu->reg, to_sub, 1);

    LOG_DEBUG("%s %s, %s\n", inst->inst_str, operand_strs[inst->op1], operand_strs[inst->op2]);
}

// the compare (CP) instruction (same as SUB but without storing the result)
void cp(gameboy *gb, gb_instruction *inst)
{
    /* NOTE: First operand of CP is always the A register
     *
     * Affected Flags
     * --------------
     * Zero Flag:         set if result is zero
     * Subtract Flag:     set
     * Half Carry Flag:   set if borrow from bit 4
     * Carry Flag:        set if borrow (set if op2 > A)
     *
     * NOTE: CP A, A will always set the zero flag
     */

    uint8_t to_sub = 0; // the value to subtract
    switch (inst->op2)
    {
        case REG_A:
            to_sub = gb->cpu->reg->a;
            break;

        case REG_B:
            to_sub = gb->cpu->reg->b;
            break;

        case REG_C:
            to_sub = gb->cpu->reg->c;
            break;

        case REG_D:
            to_sub = gb->cpu->reg->d;
            break;

        case REG_E:
            to_sub = gb->cpu->reg->e;
            break;

        case REG_H:
            to_sub = gb->cpu->reg->h;
            break;

        case REG_L:
            to_sub = gb->cpu->reg->l;
            break;

        case PTR_HL:
            to_sub = read_byte(gb, read_hl(gb->cpu->reg));
            break;

        case IMM_8:
            to_sub = read_byte(gb, (gb->cpu->reg->pc)++);
            break;

        default: // shouldn't get here
            LOG_ERROR("Illegal argument in %s encountered. Exiting...\n", inst->inst_str);
            exit(1);
    }

    // calculate the difference (without storing the result) and set flags
    sub_from_reg_a(gb->cpu->reg, to_sub, 0);

    LOG_DEBUG("%s %s, %s\n", inst->inst_str, operand_strs[inst->op1], operand_strs[inst->op2]);
}

// the bitwise and instruction
void and(gameboy *gb, gb_instruction *inst)
{
    /* NOTE: first operand of AND is always the A register
     *
     * Affected flags
     * --------------
     *  Zero Flag:         set if result is zero
     *  Subtract Flag:     reset
     *  Half Carry Flag:   set
     *  Carry Flag:        reset
     */

    uint8_t to_and = 0; // value to bitwise and with register A
    switch (inst->op2)
    {
        case REG_A:
            to_and = gb->cpu->reg->a;
            break;

        case REG_B:
            to_and = gb->cpu->reg->b;
            break;

        case REG_C:
            to_and = gb->cpu->reg->c;
            break;

        case REG_D:
            to_and = gb->cpu->reg->d;
            break;

        case REG_E:
            to_and = gb->cpu->reg->e;
            break;

        case REG_H:
            to_and = gb->cpu->reg->h;
            break;

        case REG_L:
            to_and = gb->cpu->reg->l;
            break;

        case PTR_HL:
            to_and = read_byte(gb, read_hl(gb->cpu->reg));
            break;

        case IMM_8:
            to_and = read_byte(gb, (gb->cpu->reg->pc)++);
            break;

        default: // shouldn't get here
            LOG_ERROR("Illegal argument in %s encountered. Exiting...\n", inst->inst_str);
            exit(1);
    }
    gb->cpu->reg->a &= to_and;
    set_flags(gb->cpu->reg, gb->cpu->reg->a == 0, 0, 1, 0);

    LOG_DEBUG("%s %s, %s\n", inst->inst_str, operand_strs[inst->op1], operand_strs[inst->op2]);
}

// the bitwise or instruction
void or(gameboy *gb, gb_instruction *inst)
{
    /* NOTE: first operand of OR is always the A register
     *
     * Affected flags
     * --------------
     *  Zero Flag:         set if result is zero
     *  Subtract Flag:     reset
     *  Half Carry Flag:   reset
     *  Carry Flag:        reset
     */

    uint8_t to_or = 0; // value to bitwise or with register A
    switch (inst->op2)
    {
        case REG_A:
            to_or = gb->cpu->reg->a;
            break;

        case REG_B:
            to_or = gb->cpu->reg->b;
            break;

        case REG_C:
            to_or = gb->cpu->reg->c;
            break;

        case REG_D:
            to_or = gb->cpu->reg->d;
            break;

        case REG_E:
            to_or = gb->cpu->reg->e;
            break;

        case REG_H:
            to_or = gb->cpu->reg->h;
            break;

        case REG_L:
            to_or = gb->cpu->reg->l;
            break;

        case PTR_HL:
            to_or = read_byte(gb, read_hl(gb->cpu->reg));
            break;

        case IMM_8:
            to_or = read_byte(gb, (gb->cpu->reg->pc)++);
            break;

        default: // shouldn't get here
            LOG_ERROR("Illegal argument in %s encountered. Exiting...\n", inst->inst_str);
            exit(1);
    }
    gb->cpu->reg->a |= to_or;
    set_flags(gb->cpu->reg, gb->cpu->reg->a == 0, 0, 0, 0);

    LOG_DEBUG("%s %s, %s\n", inst->inst_str, operand_strs[inst->op1], operand_strs[inst->op2]);
}

// the bitwise xor instruction
void xor(gameboy *gb, gb_instruction *inst)
{
    /* NOTE: first operand of XOR is always the A register
     * NOTE: XOR A, A will always set the zero flag
     *
     * Affected flags
     * --------------
     *  Zero Flag:         set if result is zero
     *  Subtract Flag:     reset
     *  Half Carry Flag:   reset
     *  Carry Flag:        reset
     */

    uint8_t to_xor = 0; // value to bitwise xor with register A
    switch (inst->op2)
    {
        case REG_A:
            to_xor = gb->cpu->reg->a;
            break;

        case REG_B:
            to_xor = gb->cpu->reg->b;
            break;

        case REG_C:
            to_xor = gb->cpu->reg->c;
            break;

        case REG_D:
            to_xor = gb->cpu->reg->d;
            break;

        case REG_E:
            to_xor = gb->cpu->reg->e;
            break;

        case REG_H:
            to_xor = gb->cpu->reg->h;
            break;

        case REG_L:
            to_xor = gb->cpu->reg->l;
            break;

        case PTR_HL:
            to_xor = read_byte(gb, read_hl(gb->cpu->reg));
            break;

        case IMM_8:
            to_xor = read_byte(gb, (gb->cpu->reg->pc)++);
            break;

        default: // shouldn't get here
            LOG_ERROR("Illegal argument in %s encountered. Exiting...\n", inst->inst_str);
            exit(1);
    }
    gb->cpu->reg->a ^= to_xor;
    set_flags(gb->cpu->reg, gb->cpu->reg->a == 0, 0, 0, 0);

    LOG_DEBUG("%s %s, %s\n", inst->inst_str, operand_strs[inst->op1], operand_strs[inst->op2]);
}
