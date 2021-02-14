#ifndef GB_CARTRIDGE_H
#define GB_CARTRIDGE_H

#include <stdio.h>
#include <stdint.h>

/* cartridge errors during init process */
typedef enum ROM_LOAD_STATUS {
    ROM_LOAD_SUCCESS,
    MALFORMED_ROM,
    ROM_LOAD_ERROR,
} ROM_LOAD_STATUS;

typedef struct gb_cartridge {
    /* The cartridge's ROM banks. There are a
     * minimum of 2 banks and a max of 512 banks.
     */
    uint8_t **rom_banks;

    uint16_t num_banks;
} gb_cartridge;

/* free the memory allocated for the cartridge */
void unload_cartridge(gb_cartridge *cart);

/* initialize the cartridge struct */
gb_cartridge *init_cartridge(void);

/* load a ROM file into the cartridge struct */
ROM_LOAD_STATUS load_rom(gb_cartridge *cart, FILE *rom_file);

#endif
