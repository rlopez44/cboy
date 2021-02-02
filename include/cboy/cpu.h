#ifndef CPU_H_
#define CPU_H_

#include <stdint.h>

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
} gb_cpu;


/* Utility functions for reading and writing
 * to the bc register. This register is composed
 * of register b in the high byte and register
 * c in the low byte of the two-byte register.
 */
uint16_t read_bc(gb_registers *reg);

void write_bc(gb_registers *reg, uint16_t value);

/* Similar utility functions for the af, de, and hl registers
 * ----------------------------------------------------------
 */
uint16_t read_af(gb_registers *reg);

void write_af(gb_registers *reg, uint16_t value);

uint16_t read_de(gb_registers *reg);

void write_de(gb_registers *reg, uint16_t value);

uint16_t read_hl(gb_registers *reg);

void write_hl(gb_registers *reg, uint16_t value);

// utility function for setting all flags at once
void set_flags(gb_registers *reg, uint8_t zero, uint8_t subtract,
               uint8_t half_carry, uint8_t carry);

// utility functions for setting individual flags
void set_zero_flag(gb_registers *reg, uint8_t value);

void set_subtract_flag(gb_registers *reg, uint8_t value);

void set_half_carry_flag(gb_registers *reg, uint8_t value);

void set_carry_flag(gb_registers *reg, uint8_t value);

// utility functions for reading indiidual flags
uint8_t read_zero_flag(gb_registers *reg);

uint8_t read_subtract_flag(gb_registers *reg);

uint8_t read_half_carry_flag(gb_registers *reg);

uint8_t read_carry_flag(gb_registers *reg);

#endif
