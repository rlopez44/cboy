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

/* Memory Bank Controller types.
 * See: https://gbdev.io/pandocs/#the-cartridge-header
 */
typedef enum MBC_TYPE {
    UNKNOWN_MBC,
    NO_MBC,
    MBC1,
    MBC2,
    MBC3,
    MBC5,
    MBC6,
    MBC7,
    MMM01,
    HuC1,
    HuC3,
} MBC_TYPE;

typedef struct gb_cartridge {
    /* The cartridge's ROM banks. There are a
     * minimum of 2 banks and a max of 512 banks.
     */
    uint8_t **rom_banks;
    uint16_t num_rom_banks;

    /* the cartridge's RAM banks */
    uint8_t **ram_banks;

    /* the number of RAM banks and their size in bytes */
    uint16_t num_ram_banks, ram_bank_size;

    /* the cartridge's MBC */
    MBC_TYPE mbc;
} gb_cartridge;

/* free the memory allocated for the cartridge */
void unload_cartridge(gb_cartridge *cart);

/* initialize the cartridge struct */
gb_cartridge *init_cartridge(void);

/* load a ROM file into the cartridge struct */
ROM_LOAD_STATUS load_rom(gb_cartridge *cart, FILE *rom_file);

#endif
