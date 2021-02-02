#include <stdint.h>
#include "cboy/cpu.h"

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
    reg->f = (uint8_t)(value & 0xFF);
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
void set_flags(gb_registers *reg, uint8_t zero, uint8_t subtract,
               uint8_t half_carry, uint8_t carry)
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
void set_zero_flag(gb_registers *reg, uint8_t value)
{
    // zero flag is the seventh bit of the flag register
    uint8_t mask = 1 << 7;
    reg->f = (reg->f & ~mask) | (value & mask);
}

void set_subtract_flag(gb_registers *reg, uint8_t value)
{
    // subtract flag is the sixth bit of the flag register
    uint8_t mask = 1 << 6;
    reg->f = (reg->f & ~mask) | (value & mask);
}

void set_half_carry_flag(gb_registers *reg, uint8_t value)
{
    // half carry flag is the fifth bit of the flag register
    uint8_t mask = 1 << 5;
    reg->f = (reg->f & ~mask) | (value & mask);
}

void set_carry_flag(gb_registers *reg, uint8_t value)
{
    // carry flag is the fourth bit of the flag register
    uint8_t mask = 1 << 4;
    reg->f = (reg->f & ~mask) | (value & mask);
}

// TODO: see about inlining these functions
// read individual flags
uint8_t read_zero_flag(gb_registers *reg)
{
    // seventh bit of the flags register
    return (reg->f >> 7) & 1;
}

uint8_t read_subtract_flag(gb_registers *reg)
{
    // sixth bit of the flags register
    return (reg->f >> 6) & 1;
}

uint8_t read_half_carry_flag(gb_registers *reg)
{
    // fifth bit of the flags register
    return (reg->f >> 5) & 1;
}

uint8_t read_carry_flag(gb_registers *reg)
{
    // fourth bit of the flags register
    return (reg->f >> 4) & 1;
}
