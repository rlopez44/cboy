// Implementation of the LD and LDH instruction sets

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "cboy/instructions.h"
#include "cboy/gameboy.h"
#include "cboy/cpu.h"
#include "cboy/memory.h"
#include "cboy/log.h"
#include "execute.h"

// the load instruction
void ld(gameboy *gb, gb_instruction *inst)
{
    switch (inst->op1)
    {
        case REG_A:
            switch (inst->op2)
            {
                case REG_A:
                    gb->cpu->reg->a = gb->cpu->reg->a;

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;

                case REG_B:
                    gb->cpu->reg->a = gb->cpu->reg->b;

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;

                case REG_C:
                    gb->cpu->reg->a = gb->cpu->reg->c;

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;

                case REG_D:
                    gb->cpu->reg->a = gb->cpu->reg->d;

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;

                case REG_E:
                    gb->cpu->reg->a = gb->cpu->reg->e;

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;

                case REG_H:
                    gb->cpu->reg->a = gb->cpu->reg->h;

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;

                case REG_L:
                    gb->cpu->reg->a = gb->cpu->reg->l;

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;

                case PTR_BC:
                    gb->cpu->reg->a = read_byte(gb, read_bc(gb->cpu->reg));

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;

                case PTR_DE:
                    gb->cpu->reg->a = read_byte(gb, read_de(gb->cpu->reg));

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;

                case PTR_HL:
                    gb->cpu->reg->a = read_byte(gb, read_hl(gb->cpu->reg));

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;

                case PTR_HL_INC:
                {
                    uint16_t hl = read_hl(gb->cpu->reg);
                    gb->cpu->reg->a = read_byte(gb, hl);
                    // increment HL register after loading value it points to
                    write_hl(gb->cpu->reg, hl + 1);

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;
                }

                case PTR_HL_DEC:
                {
                    uint16_t hl = read_hl(gb->cpu->reg);
                    gb->cpu->reg->a = read_byte(gb, hl);
                    // decrement HL register after loading value it points to
                    write_hl(gb->cpu->reg, hl - 1);

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;
                }

                case IMM_8:
                {
                    // load immediate value
                    uint8_t val = read_byte(gb, (gb->cpu->reg->pc)++);
                    gb->cpu->reg->a = val;

                    LOG_DEBUG("%s %s, 0x%02x\n", inst->inst_str, operand_strs[inst->op1], val);
                    break;
                }

                case PTR_16:
                {
                    // load 16-bit immediate value
                    // NOTE: little-endian
                    uint8_t lo = read_byte(gb, (gb->cpu->reg->pc)++);
                    uint8_t hi = read_byte(gb, (gb->cpu->reg->pc)++);

                    // use this value as a pointer
                    uint16_t addr = ((uint16_t)hi << 8) | ((uint16_t)lo);
                    gb->cpu->reg->a = read_byte(gb, addr);

                    LOG_DEBUG("%s %s, [0x%04x]\n", inst->inst_str, operand_strs[inst->op1], addr);
                    break;
                }

                default: // shouldn't get here
                    LOG_ERROR("Illegal argument in %s A encountered. Exiting...\n", inst->inst_str);
                    exit(1);
            }
            break;

        case REG_B:
            switch (inst->op2)
            {
                case REG_A:
                    gb->cpu->reg->b = gb->cpu->reg->a;

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;

                case REG_B:
                    gb->cpu->reg->b = gb->cpu->reg->b;

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;

                case REG_C:
                    gb->cpu->reg->b = gb->cpu->reg->c;

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;

                case REG_D:
                    gb->cpu->reg->b = gb->cpu->reg->d;

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;

                case REG_E:
                    gb->cpu->reg->b = gb->cpu->reg->e;

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;

                case REG_H:
                    gb->cpu->reg->b = gb->cpu->reg->h;

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;

                case REG_L:
                    gb->cpu->reg->b = gb->cpu->reg->l;

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;

                case PTR_HL:
                    gb->cpu->reg->b = read_byte(gb, read_hl(gb->cpu->reg));

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;

                case IMM_8:
                {
                    uint8_t val = read_byte(gb, (gb->cpu->reg->pc)++);
                    gb->cpu->reg->b = val;

                    LOG_DEBUG("%s %s, 0x%02x\n", inst->inst_str, operand_strs[inst->op1], val);
                    break;
                }

                default: // shouldn't get here
                    LOG_ERROR("Illegal argument in %s B encountered. Exiting...\n", inst->inst_str);
                    exit(1);
            }
            break;

        case REG_C:
            switch (inst->op2)
            {
                case REG_A:
                    gb->cpu->reg->c = gb->cpu->reg->a;

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;

                case REG_B:
                    gb->cpu->reg->c = gb->cpu->reg->b;

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;

                case REG_C:
                    gb->cpu->reg->c = gb->cpu->reg->c;

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;

                case REG_D:
                    gb->cpu->reg->c = gb->cpu->reg->d;

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;

                case REG_E:
                    gb->cpu->reg->c = gb->cpu->reg->e;

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;

                case REG_H:
                    gb->cpu->reg->c = gb->cpu->reg->h;

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;

                case REG_L:
                    gb->cpu->reg->c = gb->cpu->reg->l;

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;

                case PTR_HL:
                    gb->cpu->reg->c = read_byte(gb, read_hl(gb->cpu->reg));

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;

                case IMM_8:
                {
                    uint8_t val = read_byte(gb, (gb->cpu->reg->pc)++);
                    gb->cpu->reg->c = val;

                    LOG_DEBUG("%s %s, 0x%02x\n", inst->inst_str, operand_strs[inst->op1], val);
                    break;
                }

                default: // shouldn't get here
                    LOG_ERROR("Illegal argument in %s C encountered. Exiting...\n", inst->inst_str);
                    exit(1);
            }
            break;

        case REG_D:
            switch (inst->op2)
            {
                case REG_A:
                    gb->cpu->reg->d = gb->cpu->reg->a;

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;

                case REG_B:
                    gb->cpu->reg->d = gb->cpu->reg->b;

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;

                case REG_C:
                    gb->cpu->reg->d = gb->cpu->reg->c;

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;

                case REG_D:
                    gb->cpu->reg->d = gb->cpu->reg->d;

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;

                case REG_E:
                    gb->cpu->reg->d = gb->cpu->reg->e;

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;

                case REG_H:
                    gb->cpu->reg->d = gb->cpu->reg->h;

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;

                case REG_L:
                    gb->cpu->reg->d = gb->cpu->reg->l;

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;

                case PTR_HL:
                    gb->cpu->reg->d = read_byte(gb, read_hl(gb->cpu->reg));

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;

                case IMM_8:
                {
                    uint8_t val = read_byte(gb, (gb->cpu->reg->pc)++);
                    gb->cpu->reg->d = val;

                    LOG_DEBUG("%s %s, 0x%02x\n", inst->inst_str, operand_strs[inst->op1], val);
                    break;
                }

                default: // shouldn't get here
                    LOG_ERROR("Illegal argument in %s D encountered. Exiting...\n", inst->inst_str);
                    exit(1);
            }
            break;

        case REG_E:
            switch (inst->op2)
            {
                case REG_A:
                    gb->cpu->reg->e = gb->cpu->reg->a;

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;

                case REG_B:
                    gb->cpu->reg->e = gb->cpu->reg->b;

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;

                case REG_C:
                    gb->cpu->reg->e = gb->cpu->reg->c;

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;

                case REG_D:
                    gb->cpu->reg->e = gb->cpu->reg->d;

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;

                case REG_E:
                    gb->cpu->reg->e = gb->cpu->reg->e;

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;

                case REG_H:
                    gb->cpu->reg->e = gb->cpu->reg->h;

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;

                case REG_L:
                    gb->cpu->reg->e = gb->cpu->reg->l;

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;

                case PTR_HL:
                    gb->cpu->reg->e = read_byte(gb, read_hl(gb->cpu->reg));

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;

                case IMM_8:
                {
                    uint8_t val = read_byte(gb, (gb->cpu->reg->pc)++);
                    gb->cpu->reg->e = val;

                    LOG_DEBUG("%s %s, 0x%02x\n", inst->inst_str, operand_strs[inst->op1], val);
                    break;
                }

                default: // shouldn't get here
                    LOG_ERROR("Illegal argument in %s E encountered. Exiting...\n", inst->inst_str);
                    exit(1);
            }
            break;

        case REG_H:
            switch (inst->op2)
            {
                case REG_A:
                    gb->cpu->reg->h = gb->cpu->reg->a;

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;

                case REG_B:
                    gb->cpu->reg->h = gb->cpu->reg->b;

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;

                case REG_C:
                    gb->cpu->reg->h = gb->cpu->reg->c;

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;

                case REG_D:
                    gb->cpu->reg->h = gb->cpu->reg->d;

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;

                case REG_E:
                    gb->cpu->reg->h = gb->cpu->reg->e;

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;

                case REG_H:
                    gb->cpu->reg->h = gb->cpu->reg->h;

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;

                case REG_L:
                    gb->cpu->reg->h = gb->cpu->reg->l;

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;

                case PTR_HL:
                    gb->cpu->reg->h = read_byte(gb, read_hl(gb->cpu->reg));

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;

                case IMM_8:
                {
                    uint8_t val = read_byte(gb, (gb->cpu->reg->pc)++);
                    gb->cpu->reg->h = val;

                    LOG_DEBUG("%s %s, 0x%02x\n", inst->inst_str, operand_strs[inst->op1], val);
                    break;
                }

                default: // shouldn't get here
                    LOG_ERROR("Illegal argument in %s H encountered. Exiting...\n", inst->inst_str);
                    exit(1);
            }
            break;

        case REG_L:
            switch (inst->op2)
            {
                case REG_A:
                    gb->cpu->reg->l = gb->cpu->reg->a;

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;

                case REG_B:
                    gb->cpu->reg->l = gb->cpu->reg->b;

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;

                case REG_C:
                    gb->cpu->reg->l = gb->cpu->reg->c;

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;

                case REG_D:
                    gb->cpu->reg->l = gb->cpu->reg->d;

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;

                case REG_E:
                    gb->cpu->reg->l = gb->cpu->reg->e;

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;

                case REG_H:
                    gb->cpu->reg->l = gb->cpu->reg->h;

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;

                case REG_L:
                    gb->cpu->reg->l = gb->cpu->reg->l;

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;

                case PTR_HL:
                    gb->cpu->reg->l = read_byte(gb, read_hl(gb->cpu->reg));

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;

                case IMM_8:
                {
                    uint8_t val = read_byte(gb, (gb->cpu->reg->pc)++);
                    gb->cpu->reg->l = val;

                    LOG_DEBUG("%s %s, 0x%02x\n", inst->inst_str, operand_strs[inst->op1], val);
                    break;
                }

                default: // shouldn't get here
                    LOG_ERROR("Illegal argument in %s L encountered. Exiting...\n", inst->inst_str);
                    exit(1);
            }
            break;

        case PTR_HL:
            switch (inst->op2)
            {
                case REG_A:
                    write_byte(gb, read_hl(gb->cpu->reg), gb->cpu->reg->a);

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;

                case REG_B:
                    write_byte(gb, read_hl(gb->cpu->reg), gb->cpu->reg->b);

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;

                case REG_C:
                    write_byte(gb, read_hl(gb->cpu->reg), gb->cpu->reg->c);

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;

                case REG_D:
                    write_byte(gb, read_hl(gb->cpu->reg), gb->cpu->reg->d);

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;

                case REG_E:
                    write_byte(gb, read_hl(gb->cpu->reg), gb->cpu->reg->e);

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;

                case REG_H:
                    write_byte(gb, read_hl(gb->cpu->reg), gb->cpu->reg->h);

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;

                case REG_L:
                    write_byte(gb, read_hl(gb->cpu->reg), gb->cpu->reg->l);

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;

                case IMM_8:
                {
                    // store immediate value into byte pointed to by HL
                    uint8_t value = read_byte(gb, (gb->cpu->reg->pc)++);
                    write_byte(gb, read_hl(gb->cpu->reg), value);

                    LOG_DEBUG("%s %s, 0x%02x\n", inst->inst_str, operand_strs[inst->op1], value);
                    break;
                }

                default: // shouldn't get here
                    LOG_ERROR("Illegal argument in %s [HL] encountered. Exiting...\n", inst->inst_str);
                    exit(1);
            }
            break;

        case PTR_HL_INC:
        {
            // store register A's value into [HL] then increment HL
            uint16_t hl = read_hl(gb->cpu->reg);
            write_byte(gb, hl, gb->cpu->reg->a);
            write_hl(gb->cpu->reg, hl + 1);

            LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                      operand_strs[inst->op1],
                      operand_strs[inst->op2]);
            break;
        }

        case PTR_HL_DEC:
        {
            // store register A's value into [HL] then decrement HL
            uint16_t hl = read_hl(gb->cpu->reg);
            write_byte(gb, hl, gb->cpu->reg->a);
            write_hl(gb->cpu->reg, hl - 1);

            LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                      operand_strs[inst->op1],
                      operand_strs[inst->op2]);
            break;
        }

        case PTR_BC:
            write_byte(gb, read_bc(gb->cpu->reg), gb->cpu->reg->a);

            LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                      operand_strs[inst->op1],
                      operand_strs[inst->op2]);
            break;

        case PTR_DE:
            write_byte(gb, read_de(gb->cpu->reg), gb->cpu->reg->a);

            LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                      operand_strs[inst->op1],
                      operand_strs[inst->op2]);
            break;

        case REG_BC: // only instruction is LD BC, IMM_16
        {
            // little endian
            uint8_t lo = read_byte(gb, (gb->cpu->reg->pc)++);
            uint8_t hi = read_byte(gb, (gb->cpu->reg->pc)++);
            uint16_t value = ((uint16_t)hi << 8) | ((uint16_t)lo);
            write_bc(gb->cpu->reg, value);

            LOG_DEBUG("%s %s, 0x%04x\n", inst->inst_str, operand_strs[inst->op1], value);
            break;
        }

        case REG_DE: // only instruction is LD DE, IMM_16
        {
            // little endian
            uint8_t lo = read_byte(gb, (gb->cpu->reg->pc)++);
            uint8_t hi = read_byte(gb, (gb->cpu->reg->pc)++);
            uint16_t value = ((uint16_t)hi << 8) | ((uint16_t)lo);
            write_de(gb->cpu->reg, value);

            LOG_DEBUG("%s %s, 0x%04x\n", inst->inst_str, operand_strs[inst->op1], value);
            break;
        }

        case REG_HL:
            switch (inst->op2)
            {
                case IMM_16:
                {
                    // little endian
                    uint8_t lo = read_byte(gb, (gb->cpu->reg->pc)++);
                    uint8_t hi = read_byte(gb, (gb->cpu->reg->pc)++);
                    uint16_t value = ((uint16_t)hi << 8) | ((uint16_t)lo);
                    write_hl(gb->cpu->reg, value);

                    LOG_DEBUG("%s %s, 0x%04x\n", inst->inst_str, operand_strs[inst->op1], value);
                    break;
                }

                case IMM_8: // immediate value as signed offset
                {
                    uint8_t offset = read_byte(gb, (gb->cpu->reg->pc)++);
                    bool sign_bit = (offset >> 7) & 1;

                    // HL and SP are 16 bits, so we need to sign extend the offset before adding
                    uint16_t signed_offset = sign_bit ? 0xff00 | (uint16_t)offset : offset;

                    write_hl(gb->cpu->reg, gb->cpu->reg->sp + signed_offset);

                    /* Flags to set:
                     * zero flag: 0
                     * subtract flag: 0
                     * half carry flag: set if overflow from bit 3
                     * carry flag: set if overflow from bit 7
                     *
                     * NOTE: flags are set based on unsigned value of offset
                     */
                    // lowest nibbles must add to value bigger than 0xf to overflow
                    bool half_carry = (gb->cpu->reg->sp & 0xf) + (offset & 0xf) > 0xf;

                    // sum of lowest bytes must be greater than 0xff to overflow
                    bool carry = (gb->cpu->reg->sp & 0xff) + offset > 0xff;

                    set_flags(gb->cpu->reg, 0, 0, half_carry, carry);

                    LOG_DEBUG("%s %s, 0x%02x\n", inst->inst_str, operand_strs[inst->op1], offset);
                    break;
                }

                default: // shouldn't get here
                    LOG_ERROR("Illegal argument in %s HL encountered. Exiting...\n", inst->inst_str);
                    exit(1);
            }
            break;

        case REG_SP:
            switch (inst->op2)
            {
                case IMM_16:
                {
                    uint8_t lo = read_byte(gb, (gb->cpu->reg->pc)++);
                    uint8_t hi = read_byte(gb, (gb->cpu->reg->pc)++);
                    uint16_t value = ((uint16_t)hi << 8) | ((uint16_t)lo);
                    gb->cpu->reg->sp = value;

                    LOG_DEBUG("%s %s, 0x%04x\n", inst->inst_str, operand_strs[inst->op1], value);
                    break;
                }

                case REG_HL:
                    gb->cpu->reg->sp = read_hl(gb->cpu->reg);

                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;

                default: // shouldn't get here
                    LOG_ERROR("Illegal argument in %s SP encountered. Exiting...\n", inst->inst_str);
                    exit(1);
            }
            break;

        case PTR_16:
        {
            // load the immediate address
            uint8_t lo = read_byte(gb, (gb->cpu->reg->pc)++);
            uint8_t hi = read_byte(gb, (gb->cpu->reg->pc)++);
            uint16_t addr = ((uint16_t)hi << 8) | (uint16_t)lo;

            switch (inst->op2)
            {
                case REG_A:
                    write_byte(gb, addr, gb->cpu->reg->a);
                    break;

                case REG_SP:
                    // write SP into the two bytes pointed to by the immediate address
                    // NOTE: little endian. write lo byte at addr and hi byte at addr + 1
                    write_byte(gb, addr, (uint8_t)(gb->cpu->reg->sp & 0xff));
                    write_byte(gb, addr + 1, (uint8_t)(gb->cpu->reg->sp >> 8));
                    break;

                default: // shouldn't get here
                    LOG_ERROR("Illegal argument in %s [n16] encountered. Exiting...\n", inst->inst_str);
                    exit(1);
            }

            LOG_DEBUG("%s [0x%04x], %s\n", inst->inst_str, addr, operand_strs[inst->op2]);
            break;
        }

        default: // shouldn't get here
            LOG_ERROR("Illegal argument in %s encountered. Exiting...\n", inst->inst_str);
            exit(1);
    }
}

// the 'load from high page' instruction
void ldh(gameboy *gb, gb_instruction *inst)
{
    // address that will be used in the instructions,
    // either to read from or write to memory
    uint16_t addr = 0;
    switch (inst->op1)
    {
        case REG_A:
            switch (inst->op2)
            {
                case PTR_C:
                    // 0xff00 + register C gives address to read from
                    addr = 0xff00 + (uint16_t)gb->cpu->reg->c;
                    LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                              operand_strs[inst->op1],
                              operand_strs[inst->op2]);
                    break;

                case PTR_8:
                {
                    // immediate value is low byte of address
                    // low byte + 0xff00 gives full address
                    uint8_t lo = read_byte(gb, (gb->cpu->reg->pc)++);
                    addr = 0xff00 + (uint16_t)lo;
                    LOG_DEBUG("%s %s, [0x%02x]\n", inst->inst_str, operand_strs[inst->op1], lo);
                    break;
                }

                default: // shouldn't get here
                    LOG_ERROR("Illegal argument in %s A encountered. Exiting...\n", inst->inst_str);
                    exit(1);
            }
            gb->cpu->reg->a = read_byte(gb, addr);
            break;

        case PTR_C:
            // address to write to is given by adding C register to 0xff00
            addr = 0xff00 + (uint16_t)gb->cpu->reg->c;
            write_byte(gb, addr, gb->cpu->reg->a);
            LOG_DEBUG("%s %s, %s\n", inst->inst_str,
                      operand_strs[inst->op1],
                      operand_strs[inst->op2]);
            break;

        case PTR_8:
        {
            // immediate value is the low byte of the address to read from
            // the low byte added to 0xff00 gives the full 16-bit address
            uint8_t lo = read_byte(gb, (gb->cpu->reg->pc)++);
            addr = 0xff00 + (uint16_t)lo;
            write_byte(gb, addr, gb->cpu->reg->a);
            LOG_DEBUG("%s [0x%02x], %s\n", inst->inst_str, lo, operand_strs[inst->op2]);
            break;
        }

        default: // shouldn't get here
            LOG_ERROR("Illegal argument in %s encountered. Exiting...\n", inst->inst_str);
            exit(1);
    }
}
