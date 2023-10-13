#ifndef GB_CARTRIDGE_H
#define GB_CARTRIDGE_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "cboy/mbc.h"

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
    uint16_t num_rom_banks;
    uint16_t rom_banks_bitsize;

    /* the cartridge's RAM banks */
    uint8_t **ram_banks;

    /* the number of RAM banks and their size in bytes */
    uint16_t num_ram_banks, ram_bank_size;
    uint16_t ram_banks_bitsize;

    bool has_rtc;

    /* the cartridge's MBC */
    MBC_TYPE mbc_type;
    cartridge_mbc *mbc;
} gb_cartridge;

/* free the memory allocated for the cartridge */
void unload_cartridge(gb_cartridge *cart);

/* initialize the cartridge struct */
gb_cartridge *init_cartridge(void);

/* load a ROM file into the cartridge struct */
ROM_LOAD_STATUS load_rom(gb_cartridge *cart, FILE *rom_file);

/* print the ROM's title */
void print_rom_title(gb_cartridge *cart);

/* Write cartridge RAM to a file */
void save_cartridge_ram(gb_cartridge *cart, const char *romfile);

/* Import cartridge RAM from a file */
void maybe_import_cartridge_ram(gb_cartridge *cart, const char *romfile);

#endif
