#ifndef CPU_H_
#define CPU_H_

#include <stdint.h>
#include "memory.h"

// the Game Boy CPU registers
typedef struct gb_registers {
    uint8_t a;
    uint8_t b;
    uint8_t c;
    uint8_t d;
    uint8_t e;
    uint8_t f;
    uint8_t h;
    uint8_t l;
    uint16_t sp; // stack pointer
    uint16_t pc; // program counter
} gb_registers;

// the Game Boy CPU
typedef struct gb_cpu {
    // the CPU's registers
    gb_registers *reg;

    // the CPU's address bus.
    gb_address_bus *bus;
} gb_cpu;


/* Utility functions for reading and writing
 * to the bc register. This register is composed
 * of register b in the high byte and register
 * c in the low byte of the two-byte register.
 */
uint16_t get_bc(gb_registers *reg);

void set_bc(gb_registers *reg, uint16_t value);

/* Similar utility functions for the af, de, and hl registers
 * ----------------------------------------------------------
 */
uint16_t get_af(gb_registers *reg);

void set_af(gb_registers *reg, uint16_t value);

uint16_t get_de(gb_registers *reg);

void set_de(gb_registers *reg, uint16_t value);

uint16_t get_hl(gb_registers *reg);

void set_hl(gb_registers *reg, uint16_t value);

#endif
