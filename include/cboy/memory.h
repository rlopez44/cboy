#ifndef MEMORY_H_
#define MEMORY_H_

#include <stdint.h>
#include "cboy/common.h"

/* sizes in bytes */
#define OAM_SIZE 160
#define HRAM_SIZE 127

typedef struct gameboy gameboy;

// the Game Boy's internal RAM
typedef struct gb_memory {
    // two VRAM banks - second one only used in CGB mode
    uint8_t vram[2][8 * KB];

    // 8 WRAM banks - banks 2-7 only used in CGB mode
    uint8_t wram[8][4 * KB];

    uint8_t oam[OAM_SIZE];
    uint8_t hram[HRAM_SIZE];
} gb_memory;

// Utility functions for reading and writing to memory
uint8_t read_byte(gameboy *gb, uint16_t address);
void write_byte(gameboy *gb, uint16_t address, uint8_t value);

 // read data from a RAM address
 uint8_t ram_read(gameboy *gb, uint16_t address);

 // write data to a RAM address
 void ram_write(gameboy *gb, uint16_t address, uint8_t value);

// Initialize the memory struct
gb_memory *init_memory_map(void);

// Free the memory struct
void free_memory_map(gb_memory *memory);

#endif
