#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "cboy/common.h"
#include "cboy/cpu.h"
#include "cboy/log.h"

/* bc register contains the b register in the
 * high byte and the c register in the low byte
 */
uint16_t read_bc(gb_registers *reg)
{
    return ((uint16_t)reg->b << 8) | ((uint16_t)reg->c);
}

/* extract b register from the high byte and the
 * c register from the low byte of the bc register
 */
void write_bc(gb_registers *reg, uint16_t value)
{
    reg->b = (uint8_t)((value & 0xFF00) >> 8);
    reg->c = (uint8_t)(value & 0xFF);
}

/* Similar implementations for the af, de, and hl registers
 * --------------------------------------------------------
 */
uint16_t read_af(gb_registers *reg)
{
    return ((uint16_t)reg->a << 8) | ((uint16_t)reg->f);
}

void write_af(gb_registers *reg, uint16_t value)
{
    reg->a = (uint8_t)((value & 0xFF00) >> 8);
    // the bottom 4 bits of F are unused
    reg->f = (uint8_t)(value & 0xF0);
}

uint16_t read_de(gb_registers *reg)
{
    return ((uint16_t)reg->d << 8) | ((uint16_t)reg->e);
}

void write_de(gb_registers *reg, uint16_t value)
{
    reg->d = (uint8_t)((value & 0xFF00) >> 8);
    reg->e = (uint8_t)(value & 0xFF);
}

uint16_t read_hl(gb_registers *reg)
{
    return ((uint16_t)reg->h << 8) | ((uint16_t)reg->l);
}

void write_hl(gb_registers *reg, uint16_t value)
{
    reg->h = (uint8_t)((value & 0xFF00) >> 8);
    reg->l = (uint8_t)(value & 0xFF);
}

// set all flags at once
void set_flags(gb_registers *reg, bool zero, bool subtract,
               bool half_carry, bool carry)
{
    uint8_t new_flags = zero << 7
                        | subtract << 6
                        | half_carry << 5
                        | carry << 4;

    // NOTE: flags are stored in upper nibble of flags register
    // clear the flags then set the new flag values
    reg->f = (reg->f & ~0xf0) | (new_flags & 0xf0);
}

// set individual flags
void set_zero_flag(gb_registers *reg, bool value)
{
    uint8_t mask = 1 << 7;
    reg->f = (reg->f & ~mask) | (value << 7);
}

void set_subtract_flag(gb_registers *reg, bool value)
{
    uint8_t mask = 1 << 6;
    reg->f = (reg->f & ~mask) | (value << 6);
}

void set_half_carry_flag(gb_registers *reg, bool value)
{
    uint8_t mask = 1 << 5;
    reg->f = (reg->f & ~mask) | (value << 5);
}

void set_carry_flag(gb_registers *reg, bool value)
{
    uint8_t mask = 1 << 4;
    reg->f = (reg->f & ~mask) | (value << 4);
}

// read individual flags
bool read_zero_flag(gb_registers *reg)
{
    return (reg->f >> 7) & 1;
}

bool read_subtract_flag(gb_registers *reg)
{
    return (reg->f >> 6) & 1;
}

bool read_half_carry_flag(gb_registers *reg)
{
    return (reg->f >> 5) & 1;
}

bool read_carry_flag(gb_registers *reg)
{
    return (reg->f >> 4) & 1;
}

void interrupt_register_write(gb_cpu *cpu, uint16_t address, uint8_t value)
{
    // make sure the IF and IE registers' upper three bits are always set
    value = (value & 0x1f) | 0xe0;
    switch (address)
    {
        case IF_REGISTER:
            cpu->if_register = value;
            break;

        case IE_REGISTER:
            cpu->ie_register = value;
            break;

        default:
            LOG_ERROR("Expected IF/IE register access."
                      " Got address: %04x\n",
                      address);
            exit(1);
    }
}

uint8_t interrupt_register_read(gb_cpu *cpu, uint16_t address)
{
    uint8_t value;
    switch (address)
    {
        case IF_REGISTER:
            value = cpu->if_register;
            break;

        case IE_REGISTER:
            value = cpu->ie_register;
            break;

        default:
            LOG_ERROR("Expected IF/IE register access."
                      " Got address: %04x\n",
                      address);
            exit(1);
    }

    return value;
}

/* Allocate memory for the CPU struct
 * and initialize its components.
 *
 * CPU register initial values
 * ---------------------------
 *  AF:    0x01b0
 *  BC:    0x0013
 *  DE:    0x00d8
 *  HL:    0x014d
 *  SP:    0xfffe
 *  PC:    0x0100
 *
 * Returns NULL if the allocation fails.
 */
gb_cpu *init_cpu(enum GAMEBOY_MODE gb_mode)
{
    gb_cpu *cpu = malloc(sizeof(gb_cpu));

    if (cpu == NULL)
    {
        return NULL;
    }

    cpu->is_halted = false;
    cpu->halt_bug = false;

    /* The GiiBiiAdvance emulator (see References section
     * of my README for a link) sets this flag to zero
     * during initialization, so I will too.
     */
    cpu->ime_flag = false;

    // bit mapping for IF and IE: 111BBBBB
    cpu->if_register = 0xe1;
    cpu->ie_register = 0xe0;

    // only true when a EI instruction is executed
    cpu->ime_delayed_set = false;

    cpu->reg = malloc(sizeof(gb_registers));

    if (cpu->reg == NULL)
    {
        free(cpu);
        return NULL;
    }

    // set the initial register values
    if (gb_mode == GB_DMG_MODE)
    {
        write_af(cpu->reg, 0x01b0);
        write_bc(cpu->reg, 0x0013);
        write_de(cpu->reg, 0x00d8);
        write_hl(cpu->reg, 0x014d);
    }
    else
    {
        write_af(cpu->reg, 0x1180);
        write_bc(cpu->reg, 0x0000);
        write_de(cpu->reg, 0xff56);
        write_hl(cpu->reg, 0x000d);
    }

    cpu->reg->sp = 0xfffe;
    cpu->reg->pc = 0x0100;

    return cpu;
}

/* Free the allocated memory for the CPU struct */
void free_cpu(gb_cpu *cpu)
{
    if (cpu)
        free(cpu->reg);
    free(cpu);
}
