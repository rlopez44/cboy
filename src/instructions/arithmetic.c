// Implementation of the arithmetic instruction sets

#include <stdint.h>
#include "cboy/instructions.h"
#include "cboy/cpu.h"
#include "cboy/memory.h"
#include "execute.h"

// the increment instruction
void inc(gb_cpu *cpu, gb_instruction *inst)
{
    switch (inst->op1)
    {
        case REG_A:
        {
            // store old value for carry flag check
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

        default: // shouldn't get here
            break;
    }
}
//
// the decrement instruction
void dec(gb_cpu *cpu, gb_instruction *inst)
{
    switch (inst->op1)
    {
        case REG_A:
        {
            // store old value for carry flag check
            uint8_t old_a = (cpu->reg->a)--;

            set_zero_flag(cpu->reg, cpu->reg->a == 0);
            set_subtract_flag(cpu->reg, 1);
            // set if borrow from bit 4
            // occurs if lower nibble is < 1 (i.e. 0)
            set_half_carry_flag(cpu->reg, (old_a & 0xf) == 0);
            break;
        }

        case REG_B:
        {
            uint8_t old_b = (cpu->reg->b)--;

            set_zero_flag(cpu->reg, cpu->reg->b == 0);
            set_subtract_flag(cpu->reg, 1);
            // borrow from bit 4
            set_half_carry_flag(cpu->reg, (old_b & 0xf) == 0);
            break;
        }

        case REG_C:
        {
            uint8_t old_c = (cpu->reg->c)--;

            set_zero_flag(cpu->reg, cpu->reg->c == 0);
            set_subtract_flag(cpu->reg, 1);
            // borrow from bit 4
            set_half_carry_flag(cpu->reg, (old_c & 0xf) == 0);
            break;
        }

        case REG_D:
        {
            uint8_t old_d = (cpu->reg->d)--;

            set_zero_flag(cpu->reg, cpu->reg->d == 0);
            set_subtract_flag(cpu->reg, 1);
            // borrow from bit 4
            set_half_carry_flag(cpu->reg, (old_d & 0xf) == 0);
            break;
        }

        case REG_E:
        {
            uint8_t old_e = (cpu->reg->e)--;

            set_zero_flag(cpu->reg, cpu->reg->e == 0);
            set_subtract_flag(cpu->reg, 1);
            // borrow from bit 4
            set_half_carry_flag(cpu->reg, (old_e & 0xf) == 0);
            break;
        }

        case REG_H:
        {
            uint8_t old_h = (cpu->reg->h)--;

            set_zero_flag(cpu->reg, cpu->reg->h == 0);
            set_subtract_flag(cpu->reg, 1);
            // borrow from bit 4
            set_half_carry_flag(cpu->reg, (old_h & 0xf) == 0);
            break;
        }

        case REG_L:
        {
            uint8_t old_l = (cpu->reg->l)--;

            set_zero_flag(cpu->reg, cpu->reg->l == 0);
            set_subtract_flag(cpu->reg, 1);
            // borrow from bit 4
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
            // borrow from bit 4
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

        default: // shouldn't get here
            break;
    }
}
