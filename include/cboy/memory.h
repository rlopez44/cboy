#ifndef MEMORY_H_
#define MEMORY_H_

#include <stdint.h>
#include "cboy/cartridge.h"

/* 2^16 bytes */
#define MEMORY_MAP_SIZE 65536

// the Game Boy CPU's 16-bit memory map (addresses 0x0000 to 0xFFFF)
typedef struct gb_memory {
    // map to a simple array of length 2^16 for now
    uint8_t memory[MEMORY_MAP_SIZE];
} gb_memory;

// Utility functions for reading and writing to memory
uint8_t read_byte(gb_memory *memory, uint16_t address);
void write_byte(gb_memory *memory, uint16_t address, uint8_t value);

// function to initialize the memory struct
gb_memory *init_memory_map(gb_cartridge *cart);

// function to free the memory struct
void free_memory_map(gb_memory *memory);

#endif
