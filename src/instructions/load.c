// Implementation of the LD and LDH instruction sets

#include <stdint.h>
#include "cboy/instructions.h"
#include "cboy/cpu.h"
#include "cboy/memory.h"
#include "execute.h"

// the load instruction
void ld(gb_cpu *cpu, gb_instruction *inst)
{
    switch (inst->op1)
    {
        case REG_A:
            switch (inst->op2)
            {
                case REG_A:
                    cpu->reg->a = cpu->reg->a;
                    break;

                case REG_B:
                    cpu->reg->a = cpu->reg->b;
                    break;

                case REG_C:
                    cpu->reg->a = cpu->reg->c;
                    break;

                case REG_D:
                    cpu->reg->a = cpu->reg->d;
                    break;

                case REG_E:
                    cpu->reg->a = cpu->reg->e;
                    break;

                case REG_H:
                    cpu->reg->a = cpu->reg->h;
                    break;

                case REG_L:
                    cpu->reg->a = cpu->reg->l;
                    break;

                case PTR_BC:
                    cpu->reg->a = read_byte(cpu->bus, read_bc(cpu->reg));
                    break;

                case PTR_DE:
                    cpu->reg->a = read_byte(cpu->bus, read_de(cpu->reg));
                    break;

                case PTR_HL:
                    cpu->reg->a = read_byte(cpu->bus, read_hl(cpu->reg));
                    break;

                case PTR_HL_INC:
                {
                    uint16_t hl = read_hl(cpu->reg);
                    cpu->reg->a = read_byte(cpu->bus, hl);
                    // increment HL register after loading value it points to
                    write_hl(cpu->reg, hl + 1);
                    break;
                }

                case PTR_HL_DEC:
                {
                    uint16_t hl = read_hl(cpu->reg);
                    cpu->reg->a = read_byte(cpu->bus, hl);
                    // decrement HL register after loading value it points to
                    write_hl(cpu->reg, hl - 1);
                    break;
                }

                case IMM_8:
                    // load immediate value
                    cpu->reg->a = read_byte(cpu->bus, (cpu->reg->pc)++);
                    break;

                case PTR_16:
                {
                    // load 16-bit immediate value
                    // NOTE: little-endian
                    uint8_t lo = read_byte(cpu->bus, (cpu->reg->pc)++);
                    uint8_t hi = read_byte(cpu->bus, (cpu->reg->pc)++);

                    // use this value as a pointer
                    uint16_t addr = ((uint16_t)hi << 8) | ((uint16_t)lo);
                    cpu->reg->a = read_byte(cpu->bus, addr);
                    break;
                }
            }
            break;

        case REG_B:
            switch (inst->op2)
            {
                case REG_A:
                    cpu->reg->b = cpu->reg->a;
                    break;

                case REG_B:
                    cpu->reg->b = cpu->reg->b;
                    break;

                case REG_C:
                    cpu->reg->b = cpu->reg->c;
                    break;

                case REG_D:
                    cpu->reg->b = cpu->reg->d;
                    break;

                case REG_E:
                    cpu->reg->b = cpu->reg->e;
                    break;

                case REG_H:
                    cpu->reg->b = cpu->reg->h;
                    break;

                case REG_L:
                    cpu->reg->b = cpu->reg->l;
                    break;

                case PTR_HL:
                    cpu->reg->b = read_byte(cpu->bus, read_hl(cpu->reg));
                    break;

                case IMM_8:
                    cpu->reg->b = read_byte(cpu->bus, (cpu->reg->pc)++);
                    break;
            }
            break;

        case REG_C:
            switch (inst->op2)
            {
                case REG_A:
                    cpu->reg->c = cpu->reg->a;
                    break;

                case REG_B:
                    cpu->reg->c = cpu->reg->b;
                    break;

                case REG_C:
                    cpu->reg->c = cpu->reg->c;
                    break;

                case REG_D:
                    cpu->reg->c = cpu->reg->d;
                    break;

                case REG_E:
                    cpu->reg->c = cpu->reg->e;
                    break;

                case REG_H:
                    cpu->reg->c = cpu->reg->h;
                    break;

                case REG_L:
                    cpu->reg->c = cpu->reg->l;
                    break;

                case PTR_HL:
                    cpu->reg->c = read_byte(cpu->bus, read_hl(cpu->reg));
                    break;

                case IMM_8:
                    cpu->reg->c = read_byte(cpu->bus, (cpu->reg->pc)++);
                    break;
            }
            break;

        case REG_D:
            switch (inst->op2)
            {
                case REG_A:
                    cpu->reg->d = cpu->reg->a;
                    break;

                case REG_B:
                    cpu->reg->d = cpu->reg->b;
                    break;

                case REG_C:
                    cpu->reg->d = cpu->reg->c;
                    break;

                case REG_D:
                    cpu->reg->d = cpu->reg->d;
                    break;

                case REG_E:
                    cpu->reg->d = cpu->reg->e;
                    break;

                case REG_H:
                    cpu->reg->d = cpu->reg->h;
                    break;

                case REG_L:
                    cpu->reg->d = cpu->reg->l;
                    break;

                case PTR_HL:
                    cpu->reg->d = read_byte(cpu->bus, read_hl(cpu->reg));
                    break;

                case IMM_8:
                    cpu->reg->d = read_byte(cpu->bus, (cpu->reg->pc)++);
                    break;
            }
            break;

        case REG_E:
            switch (inst->op2)
            {
                case REG_A:
                    cpu->reg->e = cpu->reg->a;
                    break;

                case REG_B:
                    cpu->reg->e = cpu->reg->b;
                    break;

                case REG_C:
                    cpu->reg->e = cpu->reg->c;
                    break;

                case REG_D:
                    cpu->reg->e = cpu->reg->d;
                    break;

                case REG_E:
                    cpu->reg->e = cpu->reg->e;
                    break;

                case REG_H:
                    cpu->reg->e = cpu->reg->h;
                    break;

                case REG_L:
                    cpu->reg->e = cpu->reg->l;
                    break;

                case PTR_HL:
                    cpu->reg->e = read_byte(cpu->bus, read_hl(cpu->reg));
                    break;

                case IMM_8:
                    cpu->reg->e = read_byte(cpu->bus, (cpu->reg->pc)++);
                    break;
            }
            break;

        case REG_H:
            switch (inst->op2)
            {
                case REG_A:
                    cpu->reg->h = cpu->reg->a;
                    break;

                case REG_B:
                    cpu->reg->h = cpu->reg->b;
                    break;

                case REG_C:
                    cpu->reg->h = cpu->reg->c;
                    break;

                case REG_D:
                    cpu->reg->h = cpu->reg->d;
                    break;

                case REG_E:
                    cpu->reg->h = cpu->reg->e;
                    break;

                case REG_H:
                    cpu->reg->h = cpu->reg->h;
                    break;

                case REG_L:
                    cpu->reg->h = cpu->reg->l;
                    break;

                case PTR_HL:
                    cpu->reg->h = read_byte(cpu->bus, read_hl(cpu->reg));
                    break;

                case IMM_8:
                    cpu->reg->h = read_byte(cpu->bus, (cpu->reg->pc)++);
                    break;
            }
            break;

        case REG_L:
            switch (inst->op2)
            {
                case REG_A:
                    cpu->reg->l = cpu->reg->a;
                    break;

                case REG_B:
                    cpu->reg->l = cpu->reg->b;
                    break;

                case REG_C:
                    cpu->reg->l = cpu->reg->c;
                    break;

                case REG_D:
                    cpu->reg->l = cpu->reg->d;
                    break;

                case REG_E:
                    cpu->reg->l = cpu->reg->e;
                    break;

                case REG_H:
                    cpu->reg->l = cpu->reg->h;
                    break;

                case REG_L:
                    cpu->reg->l = cpu->reg->l;
                    break;

                case PTR_HL:
                    cpu->reg->l = read_byte(cpu->bus, read_hl(cpu->reg));
                    break;

                case IMM_8:
                    cpu->reg->l = read_byte(cpu->bus, (cpu->reg->pc)++);
                    break;
            }
            break;

        case PTR_HL:
            switch (inst->op2)
            {
                case REG_A:
                    write_byte(cpu->bus, read_hl(cpu->reg), cpu->reg->a);
                    break;

                case REG_B:
                    write_byte(cpu->bus, read_hl(cpu->reg), cpu->reg->b);
                    break;

                case REG_C:
                    write_byte(cpu->bus, read_hl(cpu->reg), cpu->reg->c);
                    break;

                case REG_D:
                    write_byte(cpu->bus, read_hl(cpu->reg), cpu->reg->d);
                    break;

                case REG_E:
                    write_byte(cpu->bus, read_hl(cpu->reg), cpu->reg->e);
                    break;

                case REG_H:
                    write_byte(cpu->bus, read_hl(cpu->reg), cpu->reg->h);
                    break;

                case REG_L:
                    write_byte(cpu->bus, read_hl(cpu->reg), cpu->reg->l);
                    break;

                case IMM_8:
                {
                    // store immediate value into byte pointed to by HL
                    uint8_t value = read_byte(cpu->bus, (cpu->reg->pc)++);
                    write_byte(cpu->bus, read_hl(cpu->reg), value);
                    break;
                }
            }
            break;

        case PTR_HL_INC:
        {
            // store register A's value into [HL] then increment HL
            uint16_t hl = read_hl(cpu->reg);
            write_byte(cpu->bus, hl, cpu->reg->a);
            write_hl(cpu->reg, hl + 1);
            break;
        }

        case PTR_HL_DEC:
        {
            // store register A's value into [HL] then decrement HL
            uint16_t hl = read_hl(cpu->reg);
            write_byte(cpu->bus, hl, cpu->reg->a);
            write_hl(cpu->reg, hl - 1);
            break;
        }

        case PTR_BC:
            write_byte(cpu->bus, read_bc(cpu->reg), cpu->reg->a);
            break;

        case PTR_DE:
            write_byte(cpu->bus, read_de(cpu->reg), cpu->reg->a);
            break;

        case REG_BC: // only instruction is LD BC, IMM_16
        {
            // little endian
            uint8_t lo = read_byte(cpu->bus, (cpu->reg->pc)++);
            uint8_t hi = read_byte(cpu->bus, (cpu->reg->pc)++);
            uint16_t value = ((uint16_t)hi << 8) | ((uint16_t)lo);
            write_bc(cpu->reg, value);
            break;
        }

        case REG_DE: // only instruction is LD DE, IMM_16
        {
            // little endian
            uint8_t lo = read_byte(cpu->bus, (cpu->reg->pc)++);
            uint8_t hi = read_byte(cpu->bus, (cpu->reg->pc)++);
            uint16_t value = ((uint16_t)hi << 8) | ((uint16_t)lo);
            write_de(cpu->reg, value);
            break;
        }

        case REG_HL:
            switch (inst->op2)
            {
                case IMM_16:
                {
                    // little endian
                    uint8_t lo = read_byte(cpu->bus, (cpu->reg->pc)++);
                    uint8_t hi = read_byte(cpu->bus, (cpu->reg->pc)++);
                    uint16_t value = ((uint16_t)hi << 8) | ((uint16_t)lo);
                    write_hl(cpu->reg, value);
                    break;
                }

                case IMM_8:
                {
                    /* read byte from memory as a signed value
                     * NOTE: the cast from unsigned to signed 8-bit integer
                     * is always safe since the two types are the same length
                     */
                    int8_t offset = (int8_t)read_byte(cpu->bus, (cpu->reg->pc)++);

                    // NOTE: possible bugs can occur due to implicit integer conversions when
                    // offset < 0. To avoid this, we perform explicit casts of SP and the offset
                    write_hl(cpu->reg, (int32_t)cpu->reg->sp + (int32_t)offset);

                    /* Flags to set:
                     * zero flag: 0
                     * subtract flag: 0
                     * half carry flag: set if overflow from bit 3
                     * carry flag: set if overflow from bit 7
                     *
                     * NOTE: overflow can only occur if offset > 0
                     */
                    uint8_t half_carry = 0, carry = 0;
                    if (offset > 0)
                    {
                        // lowest nibbles must add to value bigger than 0xf to overflow
                        half_carry = (cpu->reg->sp & 0xf) + (offset & 0xf) > 0xf;

                        // sum of lowest bytes must be greater than 0xff to overflow
                        carry = (cpu->reg->sp & 0xff) + offset > 0xff;
                    }
                    set_flags(cpu->reg, 0, 0, half_carry, carry);
                    break;
                }
            }

        case REG_SP:
            switch (inst->op2)
            {
                case IMM_16:
                {
                    uint8_t lo = read_byte(cpu->bus, (cpu->reg->pc)++);
                    uint8_t hi = read_byte(cpu->bus, (cpu->reg->pc)++);
                    cpu->reg->sp = ((uint16_t)hi << 8) | ((uint16_t)lo);
                    break;
                }

                case REG_HL:
                    cpu->reg->sp = read_hl(cpu->reg);
                    break;
            }
            break;

        case PTR_16:
        {
            // load the immediate address
            uint8_t lo = read_byte(cpu->bus, (cpu->reg->pc)++);
            uint8_t hi = read_byte(cpu->bus, (cpu->reg->pc)++);
            uint16_t addr = ((uint16_t)hi << 8) | (uint16_t)lo;

            switch (inst->op2)
            {
                case REG_A:
                    write_byte(cpu->bus, addr, cpu->reg->a);
                    break;

                case REG_SP:
                    // write SP into the two bytes pointed to by the immediate address
                    // NOTE: little endian. write lo byte at addr and hi byte at addr + 1
                    write_byte(cpu->bus, addr, (uint8_t)(cpu->reg->sp & 0xff));
                    write_byte(cpu->bus, addr + 1, (uint8_t)(cpu->reg->sp >> 8));
                    break;
            }
            break;
        }
    }
}

// the 'load from high page' instruction
void ldh(gb_cpu *cpu, gb_instruction *inst)
{
    // address that will be used in the instructions,
    // either to read from or write to memory
    uint16_t addr;
    switch (inst->op1)
    {
        case REG_A:
            switch (inst->op2)
            {
                case PTR_C:
                    // 0xff00 + register C gives address to read from
                    addr = 0xff00 + (uint16_t)cpu->reg->c;
                    break;

                case PTR_8:
                    // immediate value is low byte of address
                    // low byte + 0xff00 gives full address
                    addr = 0xff00 + (uint16_t)read_byte(cpu->bus, (cpu->reg->pc)++);
                    break;
            }
            cpu->reg->a = read_byte(cpu->bus, addr);
            break;

        case PTR_C:
            // address to write to is given by adding C register to 0xff00
            addr = 0xff00 + (uint16_t)cpu->reg->c;
            write_byte(cpu->bus, addr, cpu->reg->a);
            break;

        case PTR_8:
            // immediate value is the low byte of the address to read from
            // the low byte added to 0xff00 gives the full 16-bit address
            addr = 0xff00 + (uint16_t)read_byte(cpu->bus, (cpu->reg->pc)++);
            write_byte(cpu->bus, addr, cpu->reg->a);
            break;
    }
}
