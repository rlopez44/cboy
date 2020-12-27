#include "cpu.h"

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
