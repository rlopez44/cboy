// Implementation of the arithmetic instruction sets

#include <stdint.h>
#include <stdbool.h>
#include "cboy/instructions.h"
#include "cboy/cpu.h"
#include "cboy/memory.h"
#include "execute.h"

// the increment instruction
void inc(gb_cpu *cpu, gb_instruction *inst)
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
            uint8_t old_a = (cpu->reg->a)++;

            set_zero_flag(cpu->reg, cpu->reg->a == 0);
            set_subtract_flag(cpu->reg, 0);
            set_half_carry_flag(cpu->reg, (old_a & 0xf) + 1 > 0xf);
            break;
        }

        case REG_B:
        {
            uint8_t old_b = (cpu->reg->b)++;

            set_zero_flag(cpu->reg, cpu->reg->b == 0);
            set_subtract_flag(cpu->reg, 0);
            set_half_carry_flag(cpu->reg, (old_b & 0xf) + 1 > 0xf);
            break;
        }

        case REG_C:
        {
            uint8_t old_c = (cpu->reg->c)++;

            set_zero_flag(cpu->reg, cpu->reg->c == 0);
            set_subtract_flag(cpu->reg, 0);
            set_half_carry_flag(cpu->reg, (old_c & 0xf) + 1 > 0xf);
            break;
        }

        case REG_D:
        {
            uint8_t old_d = (cpu->reg->d)++;

            set_zero_flag(cpu->reg, cpu->reg->d == 0);
            set_subtract_flag(cpu->reg, 0);
            set_half_carry_flag(cpu->reg, (old_d & 0xf) + 1 > 0xf);
            break;
        }

        case REG_E:
        {
            uint8_t old_e = (cpu->reg->e)++;

            set_zero_flag(cpu->reg, cpu->reg->e == 0);
            set_subtract_flag(cpu->reg, 0);
            set_half_carry_flag(cpu->reg, (old_e & 0xf) + 1 > 0xf);
            break;
        }

        case REG_H:
        {
            uint8_t old_h = (cpu->reg->h)++;

            set_zero_flag(cpu->reg, cpu->reg->h == 0);
            set_subtract_flag(cpu->reg, 0);
            set_half_carry_flag(cpu->reg, (old_h & 0xf) + 1 > 0xf);
            break;
        }

        case REG_L:
        {
            uint8_t old_l = (cpu->reg->l)++;

            set_zero_flag(cpu->reg, cpu->reg->l == 0);
            set_subtract_flag(cpu->reg, 0);
            set_half_carry_flag(cpu->reg, (old_l & 0xf) + 1 > 0xf);
            break;
        }

        case PTR_HL:
        {
            uint16_t addr = read_hl(cpu->reg);
            uint8_t old_val = read_byte(cpu->bus, addr);
            write_byte(cpu->bus, addr, old_val + 1);

            set_zero_flag(cpu->reg, old_val + 1 == 0);
            set_subtract_flag(cpu->reg, 0);
            set_half_carry_flag(cpu->reg, (old_val & 0xf) + 1 > 0xf);
            break;
        }

        case REG_BC:
            write_bc(cpu->reg, read_bc(cpu->reg) + 1);
            break;

        case REG_DE:
            write_de(cpu->reg, read_de(cpu->reg) + 1);
            break;

        case REG_HL:
            write_hl(cpu->reg, read_hl(cpu->reg) + 1);
            break;

        case REG_SP:
            ++(cpu->reg->sp);
            break;
    }
}

// the decrement instruction
void dec(gb_cpu *cpu, gb_instruction *inst)
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
            uint8_t old_a = (cpu->reg->a)--;

            set_zero_flag(cpu->reg, cpu->reg->a == 0);
            set_subtract_flag(cpu->reg, 1);
            // borrow from bit 4 occurs if lower nibble is < 1 (i.e. 0)
            set_half_carry_flag(cpu->reg, (old_a & 0xf) == 0);
            break;
        }

        case REG_B:
        {
            uint8_t old_b = (cpu->reg->b)--;

            set_zero_flag(cpu->reg, cpu->reg->b == 0);
            set_subtract_flag(cpu->reg, 1);
            set_half_carry_flag(cpu->reg, (old_b & 0xf) == 0);
            break;
        }

        case REG_C:
        {
            uint8_t old_c = (cpu->reg->c)--;

            set_zero_flag(cpu->reg, cpu->reg->c == 0);
            set_subtract_flag(cpu->reg, 1);
            set_half_carry_flag(cpu->reg, (old_c & 0xf) == 0);
            break;
        }

        case REG_D:
        {
            uint8_t old_d = (cpu->reg->d)--;

            set_zero_flag(cpu->reg, cpu->reg->d == 0);
            set_subtract_flag(cpu->reg, 1);
            set_half_carry_flag(cpu->reg, (old_d & 0xf) == 0);
            break;
        }

        case REG_E:
        {
            uint8_t old_e = (cpu->reg->e)--;

            set_zero_flag(cpu->reg, cpu->reg->e == 0);
            set_subtract_flag(cpu->reg, 1);
            set_half_carry_flag(cpu->reg, (old_e & 0xf) == 0);
            break;
        }

        case REG_H:
        {
            uint8_t old_h = (cpu->reg->h)--;

            set_zero_flag(cpu->reg, cpu->reg->h == 0);
            set_subtract_flag(cpu->reg, 1);
            set_half_carry_flag(cpu->reg, (old_h & 0xf) == 0);
            break;
        }

        case REG_L:
        {
            uint8_t old_l = (cpu->reg->l)--;

            set_zero_flag(cpu->reg, cpu->reg->l == 0);
            set_subtract_flag(cpu->reg, 1);
            set_half_carry_flag(cpu->reg, (old_l & 0xf) == 0);
            break;
        }

        case PTR_HL:
        {
            uint16_t addr = read_hl(cpu->reg);
            uint8_t old_val = read_byte(cpu->bus, addr);
            write_byte(cpu->bus, addr, old_val - 1);

            set_zero_flag(cpu->reg, old_val - 1 == 0);
            set_subtract_flag(cpu->reg, 1);
            set_half_carry_flag(cpu->reg, (old_val & 0xf) == 0);
            break;
        }

        case REG_BC:
            write_bc(cpu->reg, read_bc(cpu->reg) - 1);
            break;

        case REG_DE:
            write_de(cpu->reg, read_de(cpu->reg) - 1);
            break;

        case REG_HL:
            write_hl(cpu->reg, read_hl(cpu->reg) - 1);
            break;

        case REG_SP:
            --(cpu->reg->sp);
            break;
    }
}

// the add instruction
void add(gb_cpu *cpu, gb_instruction *inst)
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
            uint8_t to_add; // value to add
            switch (inst->op2)
            {
                case REG_A:
                    to_add = cpu->reg->a;
                    break;

                case REG_B:
                    to_add = cpu->reg->b;
                    break;

                case REG_C:
                    to_add = cpu->reg->c;
                    break;

                case REG_D:
                    to_add = cpu->reg->d;
                    break;

                case REG_E:
                    to_add = cpu->reg->e;
                    break;

                case REG_H:
                    to_add = cpu->reg->h;
                    break;

                case REG_L:
                    to_add = cpu->reg->l;
                    break;

                case PTR_HL:
                    to_add = read_byte(cpu->bus, read_hl(cpu->reg));
                    break;

                case IMM_8:
                    to_add = read_byte(cpu->bus, (cpu->reg->sp)++);
                    break;
            }
            uint8_t old_a = cpu->reg->a;
            cpu->reg->a += to_add;
            set_flags(cpu->reg,
                      cpu->reg->a == 0,                           // zero
                      0,                                          // subtract
                      (old_a & 0xf) + (to_add & 0xf) > 0xf,       // half carry
                      (uint16_t)old_a + (uint16_t)to_add > 0xff); // carry
            break;
        }

        case REG_HL:
        {
            uint16_t to_add; // value to add
            switch (inst->op2)
            {
                case REG_BC:
                    to_add = read_bc(cpu->reg);
                    break;

                case REG_DE:
                    to_add = read_de(cpu->reg);
                    break;

                case REG_HL:
                    to_add = read_hl(cpu->reg);
                    break;

                case REG_SP:
                    to_add = cpu->reg->sp;
            }
            uint16_t old_hl = read_hl(cpu->reg);
            write_hl(cpu->reg, old_hl + to_add);
            set_zero_flag(cpu->reg, 0);
            set_half_carry_flag(cpu->reg, (old_hl & 0xfff) + (to_add & 0xfff) > 0xfff);
            set_carry_flag(cpu->reg, (uint32_t)old_hl + (uint32_t)to_add > 0xffff);
            break;
        }

        case REG_SP: // single case, add signed 8-bit offset
        {
            // cast is safe since we don't change integer width
            int8_t offset = (int8_t)read_byte(cpu->bus, (cpu->reg->pc)++);
            // explicit casts to avoid bugs due to implicit integer conversions
            cpu->reg->sp = (int32_t)cpu->reg->sp + (int32_t)offset;

            uint8_t half_carry = 0, carry = 0;
            // overflow can occur only if offset > 0
            if (offset > 0)
            {
                half_carry = (cpu->reg->sp & 0xf) + (offset & 0xf) > 0xf;
                carry = (cpu->reg->sp & 0xff) + offset > 0xff;
            }
            set_flags(cpu->reg, 0, 0, half_carry, carry);
            break;
        }
    }
}

// the add with carry (ADC) instruction
void adc(gb_cpu *cpu, gb_instruction *inst)
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

    uint8_t to_add = read_carry_flag(cpu->reg); // initialize to value of carry flag
    switch (inst->op2)
    {
        case REG_A:
            to_add += cpu->reg->a;
            break;

        case REG_B:
            to_add += cpu->reg->b;
            break;

        case REG_C:
            to_add += cpu->reg->c;
            break;

        case REG_D:
            to_add += cpu->reg->d;
            break;

        case REG_E:
            to_add += cpu->reg->e;
            break;

        case REG_H:
            to_add += cpu->reg->h;
            break;

        case REG_L:
            to_add += cpu->reg->l;
            break;

        case PTR_HL:
            to_add += read_byte(cpu->bus, read_hl(cpu->reg));
            break;

        case IMM_8:
            to_add += read_byte(cpu->bus, (cpu->reg->pc)++);
            break;
    }
    uint8_t old_a = cpu->reg->a;
    cpu->reg->a += to_add;
    set_flags(cpu->reg,
              cpu->reg->a == 0,                           // zero
              0,                                          // subtract
              (old_a & 0xf) + (to_add & 0xf) > 0xf,       // half carry
              (uint16_t)old_a + (uint16_t)to_add > 0xff); // carry
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
void sub(gb_cpu *cpu, gb_instruction *inst)
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

    uint8_t to_sub; // the value to subtract
    switch (inst->op2)
    {
        case REG_A:
            to_sub = cpu->reg->a;
            break;

        case REG_B:
            to_sub = cpu->reg->b;
            break;

        case REG_C:
            to_sub = cpu->reg->c;
            break;

        case REG_D:
            to_sub = cpu->reg->d;
            break;

        case REG_E:
            to_sub = cpu->reg->e;
            break;

        case REG_H:
            to_sub = cpu->reg->h;
            break;

        case REG_L:
            to_sub = cpu->reg->l;
            break;

        case PTR_HL:
            to_sub = read_byte(cpu->bus, read_hl(cpu->reg));
            break;

        case IMM_8:
            to_sub = read_byte(cpu->bus, (cpu->reg->pc)++);
            break;
    }

    // calculate and store the difference and set flags
    sub_from_reg_a(cpu->reg, to_sub, 1);
}

// the subtract with carry (SBC) instruction
void sbc(gb_cpu *cpu, gb_instruction *inst)
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

    uint8_t to_sub = read_carry_flag(cpu->reg); // initialize to value of carry flag
    switch (inst->op2)
    {
        case REG_A:
            to_sub += cpu->reg->a;
            break;

        case REG_B:
            to_sub += cpu->reg->b;
            break;

        case REG_C:
            to_sub += cpu->reg->c;
            break;

        case REG_D:
            to_sub += cpu->reg->d;
            break;

        case REG_E:
            to_sub += cpu->reg->e;
            break;

        case REG_H:
            to_sub += cpu->reg->h;
            break;

        case REG_L:
            to_sub += cpu->reg->l;
            break;

        case PTR_HL:
            to_sub += read_byte(cpu->bus, read_hl(cpu->reg));
            break;

        case IMM_8:
            to_sub += read_byte(cpu->bus, (cpu->reg->pc)++);
            break;
    }

    // calculate and store the difference and set flags
    sub_from_reg_a(cpu->reg, to_sub, 1);
}

// the compare (CP) instruction (same as SUB but without storing the result)
void cp(gb_cpu *cpu, gb_instruction *inst)
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

    uint8_t to_sub; // the value to subtract
    switch (inst->op2)
    {
        case REG_A:
            to_sub = cpu->reg->a;
            break;

        case REG_B:
            to_sub = cpu->reg->b;
            break;

        case REG_C:
            to_sub = cpu->reg->c;
            break;

        case REG_D:
            to_sub = cpu->reg->d;
            break;

        case REG_E:
            to_sub = cpu->reg->e;
            break;

        case REG_H:
            to_sub = cpu->reg->h;
            break;

        case REG_L:
            to_sub = cpu->reg->l;
            break;

        case PTR_HL:
            to_sub = read_byte(cpu->bus, read_hl(cpu->reg));
            break;

        case IMM_8:
            to_sub = read_byte(cpu->bus, (cpu->reg->pc)++);
            break;
    }

    // calculate the difference (without storing the result) and set flags
    sub_from_reg_a(cpu->reg, to_sub, 0);
}

// the bitwise and instruction
void and(gb_cpu *cpu, gb_instruction *inst)
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

    uint8_t to_and; // value to bitwise and with register A
    switch (inst->op2)
    {
        case REG_A:
            to_and = cpu->reg->a;
            break;

        case REG_B:
            to_and = cpu->reg->b;
            break;

        case REG_C:
            to_and = cpu->reg->c;
            break;

        case REG_D:
            to_and = cpu->reg->d;
            break;

        case REG_E:
            to_and = cpu->reg->e;
            break;

        case REG_H:
            to_and = cpu->reg->h;
            break;

        case REG_L:
            to_and = cpu->reg->l;
            break;

        case PTR_HL:
            to_and = read_byte(cpu->bus, read_hl(cpu->reg));
            break;

        case IMM_8:
            to_and = read_byte(cpu->bus, (cpu->reg->pc)++);
            break;
    }
    cpu->reg->a &= to_and;
    set_flags(cpu->reg, cpu->reg->a == 0, 0, 1, 0);
}

// the bitwise or instruction
void or(gb_cpu *cpu, gb_instruction *inst)
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

    uint8_t to_or; // value to bitwise or with register A
    switch (inst->op2)
    {
        case REG_A:
            to_or = cpu->reg->a;
            break;

        case REG_B:
            to_or = cpu->reg->b;
            break;

        case REG_C:
            to_or = cpu->reg->c;
            break;

        case REG_D:
            to_or = cpu->reg->d;
            break;

        case REG_E:
            to_or = cpu->reg->e;
            break;

        case REG_H:
            to_or = cpu->reg->h;
            break;

        case REG_L:
            to_or = cpu->reg->l;
            break;

        case PTR_HL:
            to_or = read_byte(cpu->bus, read_hl(cpu->reg));
            break;

        case IMM_8:
            to_or = read_byte(cpu->bus, (cpu->reg->pc)++);
            break;
    }
    cpu->reg->a |= to_or;
    set_flags(cpu->reg, cpu->reg->a == 0, 0, 0, 0);
}

// the bitwise xor instruction
void xor(gb_cpu *cpu, gb_instruction *inst)
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

    uint8_t to_xor; // value to bitwise xor with register A
    switch (inst->op2)
    {
        case REG_A:
            to_xor = cpu->reg->a;
            break;

        case REG_B:
            to_xor = cpu->reg->b;
            break;

        case REG_C:
            to_xor = cpu->reg->c;
            break;

        case REG_D:
            to_xor = cpu->reg->d;
            break;

        case REG_E:
            to_xor = cpu->reg->e;
            break;

        case REG_H:
            to_xor = cpu->reg->h;
            break;

        case REG_L:
            to_xor = cpu->reg->l;
            break;

        case PTR_HL:
            to_xor = read_byte(cpu->bus, read_hl(cpu->reg));
            break;

        case IMM_8:
            to_xor = read_byte(cpu->bus, (cpu->reg->pc)++);
            break;
    }
    cpu->reg->a ^= to_xor;
    set_flags(cpu->reg, cpu->reg->a == 0, 0, 0, 0);
}
