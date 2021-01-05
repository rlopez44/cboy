#ifndef MEMORY_H_
#define MEMORY_H_

#include <stdint.h>

// the Game Boy CPU's 16-bit memory bus (addresses 0x0000 to 0xFFFF)
typedef struct gb_address_bus {
    // map to a simple array of length 2^16 for now
    uint8_t memory[1 << 16];
} gb_address_bus;

// Utility functions for reading and writing to memory
uint8_t read_byte(gb_address_bus *bus, uint16_t address);
void write_byte(gb_address_bus *bus, uint16_t address, uint8_t value);

#endif
