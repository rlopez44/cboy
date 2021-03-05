#ifndef MEMORY_H_
#define MEMORY_H_

#include <stdint.h>
#include "cboy/cartridge.h"

/* 2^16 bytes */
#define MEMORY_MAP_SIZE 65536

/* This header file needs to be included in gameboy.h
 * to define the gameboy type, so we need to forward
 * declare this type for the read_byte and write_byte
 * functions below.
 */
typedef struct gameboy gameboy;

// the Game Boy CPU's 16-bit memory map (addresses 0x0000 to 0xFFFF)
typedef struct gb_memory {
    // map to a simple array of length 2^16 for now
    uint8_t mmap[MEMORY_MAP_SIZE];
} gb_memory;

// Utility functions for reading and writing to memory
uint8_t read_byte(gameboy *gb, uint16_t address);
void write_byte(gameboy *gb, uint16_t address, uint8_t value);

// function to initialize the memory struct
gb_memory *init_memory_map(gb_cartridge *cart);

// function to free the memory struct
void free_memory_map(gb_memory *memory);

#endif
