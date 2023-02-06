#ifndef GB_CARTRIDGE_H
#define GB_CARTRIDGE_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

/* 16 KB (2^14 bytes) */
#define ROM_BANK_SIZE 16384

/* 8 KB (2^13 bytes) */
#define RAM_BANK_SIZE 8192

#define MAX_ROM_BANKS 512

#define MAX_RAM_BANKS 16

/* 1 KB = 2^10 bytes */
#define KB 1024

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

/* Memory Bank Controller registers */
typedef enum MBC_REGISTER {
    RAM_ENABLE,
    ROM_BANKNO,
    RAM_BANKNO,
    BANK_MODE,
} MBC_REGISTER;

typedef struct cartridge_mbc {
    bool ram_enabled;
    uint8_t rom_bankno;
    uint8_t ram_bankno;
    bool bank_mode;
} cartridge_mbc;

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
    MBC_TYPE mbc_type;
    cartridge_mbc *mbc;
} gb_cartridge;

/* free the memory allocated for the cartridge */
void unload_cartridge(gb_cartridge *cart);

/* initialize the cartridge struct */
gb_cartridge *init_cartridge(void);

/* load a ROM file into the cartridge struct */
ROM_LOAD_STATUS load_rom(gb_cartridge *cart, FILE *rom_file);

/* utility function for printing out the cartridge MBC type */
void print_mbc_type(gb_cartridge *cart);

/* forward declaration needed for handle_mbc_writes() */
typedef struct gameboy gameboy;

/* Handle MBC-related writes to memory */
void handle_mbc_writes(gameboy *gb, MBC_REGISTER reg, uint8_t val);

#endif
