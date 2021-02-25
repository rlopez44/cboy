#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "cboy/cartridge.h"

/* free the allocated memory for the cartridge struct */
void unload_cartridge(gb_cartridge *cart)
{
    // free the ROM banks
    for (int i = 0; i < cart->num_rom_banks; ++i)
    {
        free(cart->rom_banks[i]);
    }

    // free the ROM banks array
    free(cart->rom_banks);

    // free the RAM banks, if any
    for (int i = 0; i < cart->num_ram_banks; ++i)
    {
        free(cart->ram_banks[i]);
    }

    // free the RAM banks array, if any
    free(cart->ram_banks);

    // free the cartridge
    free(cart);
}

/* Helper function for loading the ROM from memory.
 * Loads a ROM bank from the file into the given
 * buffer.
 *
 * Returns a status code indicating whether
 * the bank was loaded successfully.
 */
static ROM_LOAD_STATUS load_rom_bank(uint8_t *buffer, FILE *rom_file)
{
    /* try to load a ROM bank from the ROM file */
    if (ROM_BANK_SIZE != fread(buffer, sizeof(uint8_t), ROM_BANK_SIZE, rom_file))
    {
        // either an I/O error occurred, or the ROM is malformed
        // since there weren't enough bytes to make up the ROM bank
        if (ferror(rom_file)) /* I/O error */
        {
            return ROM_LOAD_ERROR;
        }
        /* unexpected EOF */
        return MALFORMED_ROM;
    }

    return ROM_LOAD_SUCCESS;
}

/* Allocates memory for the cartridge, its ROM banks
 * array, and the ROM banks themselves. Since there
 * are a max of 512 banks, we allocate this max number.
 * The number of banks can then be shrunk later if not
 * all of them are needed. Initializes all pointers
 * in the struct to NULL.
 *
 * Returns NULL if the allocation failed.
 */

gb_cartridge *init_cartridge(void)
{
    gb_cartridge *cart = malloc(sizeof(gb_cartridge));

    // cartridge allocation failed
    if (cart == NULL)
    {
        return NULL;
    }

    // allocate the ROM and RAM banks array
    cart->rom_banks = malloc(MAX_ROM_BANKS * sizeof(uint8_t *));
    cart->ram_banks = malloc(MAX_RAM_BANKS * sizeof(uint8_t *));

    // allocation failed for ROM and/or RAM banks array
    if (cart->rom_banks == NULL || cart->ram_banks == NULL)
    {
        free(cart->rom_banks);
        free(cart->ram_banks);
        free(cart);
        return NULL;
    }

    // allocate the ROM banks and initialize values to zero
    int num_alloced_banks = 0;
    for (int i = 0; i < MAX_ROM_BANKS; ++i)
    {
        cart->rom_banks[i] = calloc(ROM_BANK_SIZE, sizeof(uint8_t));

        // ROM bank allocation failed
        if (cart->rom_banks[i] == NULL)
        {
            break;
        }

        // bank successfully allocated
        ++num_alloced_banks;
    }

    // one (or more) ROM banks failed to be allocated
    if (num_alloced_banks < MAX_ROM_BANKS)
    {
        // free the successfully alloced banks
        for (int i = 0; i < num_alloced_banks; ++i)
        {
            free(cart->rom_banks[i]);
        }

        free(cart->rom_banks);
        free(cart);
        return NULL;
    }
    // ROM banks allocation successful
    cart->num_rom_banks = MAX_ROM_BANKS;

    /* Allocate the RAM banks and initialize value to zero.
     * We allocate MAX_RAM_BANKS banks of size 8KB each.
     * When the ROM file is being loaded, the number of
     * banks and bank size will need to be adjusted
     * according to the ROM's specification.
     */
    num_alloced_banks = 0;
    for (int i = 0; i < MAX_RAM_BANKS; ++i)
    {
        cart->ram_banks[i] = calloc(8 * KB, sizeof(uint8_t));

        // RAM bank allocation failed
        if (cart->ram_banks[i] == NULL)
        {
            break;
        }

        // bank successfully allocated
        ++num_alloced_banks;
    }

    // one (or more) RAM banks failed to be allocated
    if (num_alloced_banks < MAX_RAM_BANKS)
    {
        cart->num_ram_banks = num_alloced_banks;
        unload_cartridge(cart);
        return NULL;
    }

    // all allocations were successful
    cart->num_ram_banks = MAX_RAM_BANKS;
    cart->ram_bank_size = 8 * KB;
    return cart;
}

/* determine the number of banks in the ROM given the zeroth ROM bank */
static uint16_t get_num_rom_banks(uint8_t *rom0)
{
    // byte 0x148 tells us the number of banks.
    uint16_t num_rom_banks;
    switch (rom0[0x148])
    {
        case 0x00:
            num_rom_banks = 2;
            break;
        case 0x01:
            num_rom_banks = 4;
            break;
        case 0x02:
            num_rom_banks = 8;
            break;
        case 0x03:
            num_rom_banks = 16;
            break;
        case 0x04:
            num_rom_banks = 32;
            break;
        case 0x05:
            num_rom_banks = 64;
            break;
        case 0x06:
            num_rom_banks = 128;
            break;
        case 0x07:
            num_rom_banks = 256;
            break;
        case 0x08:
            num_rom_banks = 512;
            break;
        case 0x52:
            num_rom_banks = 72;
            break;
        case 0x53:
            num_rom_banks = 80;
            break;
        case 0x54:
            num_rom_banks = 96;
            break;
        default: // not a valid value at this byte
            num_rom_banks = 0;
            break;
    }
    return num_rom_banks;
}

/* determine the MBC type given the zeroth ROM bank */
static MBC_TYPE get_mbc_type(uint8_t *rom0)
{
    // byte 0x147 tells us the MBC type.
    MBC_TYPE mbc;
    switch (rom0[0x147])
    {
        case 0x00:
        case 0x08:
        case 0x09:
        case 0xfc:
        case 0xfd:
            mbc = NO_MBC;
            break;

        case 0x01:
        case 0x02:
        case 0x03:
            mbc = MBC1;
            break;

        case 0x05:
        case 0x06:
            mbc = MBC2;
            break;

        case 0x0b:
        case 0x0c:
        case 0x0d:
            mbc = MMM01;
            break;

        case 0x0f:
        case 0x10:
        case 0x11:
        case 0x12:
        case 0x13:
            mbc = MBC3;
            break;

        case 0x19:
        case 0x1a:
        case 0x1b:
        case 0x1c:
        case 0x1d:
        case 0x1e:
            mbc = MBC5;
            break;

        case 0x20:
            mbc = MBC6;
            break;

        case 0x22:
            mbc = MBC7;
            break;

        default: // invalid byte
            mbc = UNKNOWN_MBC;
            break;
    }
    return mbc;
}

/* determine the external RAM size in KB */
static int16_t get_ext_ram_size(uint8_t *rom0)
{
    // byte 0x149 tells us the external RAM size
    int16_t ram_size_kb;
    switch (rom0[0x149])
    {
        case 0x00:
            ram_size_kb = 0;
            break;
        case 0x01:
            ram_size_kb = 2;
            break;
        case 0x02:
            ram_size_kb = 8;
            break;
        case 0x03:
            ram_size_kb = 32;
            break;
        case 0x04:
            ram_size_kb = 128;
            break;
        case 0x05:
            ram_size_kb = 64;
            break;
        default: // invalid byte
            ram_size_kb = -1;
            break;
    }
    return ram_size_kb;
}

/* determine the number of RAM banks in the cartridge */
static uint16_t get_num_ram_banks(uint8_t ext_ram_size_kb)
{
    // special case of one partial RAM bank of size 2 KB (rather than full 8 KB)
    if (ext_ram_size_kb == 2)
    {
        return 1;
    }

    return ext_ram_size_kb / 8;
}

/* Loads the ROM file into the cartridge passed in.
 * Assumes the cartridge has already been initialized
 * by init_cartridge.
 *
 * Returns a status code indicating whether the ROM
 * was loaded successfully.
 */
ROM_LOAD_STATUS load_rom(gb_cartridge *cart, FILE *rom_file)
{
    // buffer for loading the ROM banks
    uint8_t rom_bank_buffer[ROM_BANK_SIZE] = {0};

    /* The first 16 KB (2^14 bytes) are guaranteed
     * to be present in the ROM, since this is
     * the zeroth ROM bank. The cartridge header
     * is located here, and we will use its
     * information to determine the total ROM size,
     * allowing us to load the rest of the ROM.
     *
     * We check here if we were able to load the
     * full ROM bank 0 and return an error code
     * if we couldn't.
     */
    ROM_LOAD_STATUS bank_load_status = load_rom_bank(rom_bank_buffer, rom_file);

    if (bank_load_status != ROM_LOAD_SUCCESS)
    {
        return bank_load_status;
    }

    // get the MBC type from the ROM header
    cart->mbc = get_mbc_type(rom_bank_buffer);

    if (cart->mbc == UNKNOWN_MBC)
    {
        return MALFORMED_ROM;
    }

    // get the external RAM size in KB
    int16_t ext_ram_size_kb = get_ext_ram_size(rom_bank_buffer);

    if (ext_ram_size_kb == -1)
    {
        return MALFORMED_ROM;
    }

    // use the header info to load the rest of the ROM
    uint16_t num_rom_banks = get_num_rom_banks(rom_bank_buffer);

    // invalid ROM size byte
    if (!num_rom_banks)
    {
        return MALFORMED_ROM;
    }

    // copy ROM bank 0 from buffer to the actual bank
    memcpy(cart->rom_banks[0], rom_bank_buffer, ROM_BANK_SIZE * sizeof(uint8_t));

    // copy the remaining ROM banks
    for (int i = 1; i < num_rom_banks; ++i)
    {
        bank_load_status = load_rom_bank(rom_bank_buffer, rom_file);

        // I/O error or malformed ROM
        if (bank_load_status != ROM_LOAD_SUCCESS)
        {
            return bank_load_status;
        }

        // bank copied successfully, so move it out of the buffer
        memcpy(cart->rom_banks[i], rom_bank_buffer, ROM_BANK_SIZE * sizeof(uint8_t));
    }


    /* All banks copied successfully, so resize ROM banks array if needed.
     * Since the cartridge is initialized with the maximum number of ROM
     * banks, we only need to shrink down the ROM banks array.
     */
    if (num_rom_banks != MAX_ROM_BANKS)
    {
        // free the unused banks before resizing the banks array
        for (int i = num_rom_banks; i < MAX_ROM_BANKS; ++i)
        {
            free(cart->rom_banks[i]);
        }

        uint8_t **tmp = realloc(cart->rom_banks, num_rom_banks * sizeof(uint8_t *));

        if (tmp == NULL)
        {
            return ROM_LOAD_ERROR;
        }
        else if (tmp != cart->rom_banks)
        {
            cart->rom_banks = tmp;
        }
        tmp = NULL;

        // realloc was successful, so we update the number of banks in the cartridge
        cart->num_rom_banks = num_rom_banks;
    }

    // resize to appropriate number of RAM banks of appropriate size
    uint16_t ram_bank_size, num_ram_banks = get_num_ram_banks((uint8_t)ext_ram_size_kb);

    if (!ext_ram_size_kb)
    {
        ram_bank_size = 0;
    }
    else
    {
        ram_bank_size = ext_ram_size_kb == 2 ? 2 * KB : 8 * KB;
    }

    if (num_ram_banks != MAX_RAM_BANKS)
    {
        // free the unused banks before resizing the banks array
        for (int i = num_ram_banks; i < MAX_RAM_BANKS; ++i)
        {
            free(cart->ram_banks[i]);
        }

        uint8_t **tmp = realloc(cart->ram_banks, num_ram_banks * sizeof(uint8_t *));

        /* NOTE: unlike for the ROM banks array, it is possible for the
         * cartridge to have no external RAM. If this is the case, the
         * realloc call directly above will simply free the RAM banks array
         * and return NULL. In this special case, the NULL is expected,
         * and does not indicate that an error occurred. Hence, we must
         * check that NULL was returned and that we expected a non-empty
         * RAM banks array before returning an error code.
         */
        if (tmp == NULL && num_ram_banks)
        {
            return ROM_LOAD_ERROR;
        }
        else if (tmp != cart->ram_banks)
        {
            cart->ram_banks = tmp;
        }
        tmp = NULL;

        // realloc was successful, so update number of RAM banks and their size
        cart->num_ram_banks = num_ram_banks;
        cart->ram_bank_size = ram_bank_size;
    }

    return ROM_LOAD_SUCCESS;
}

/* print out the cartridge MBC type */
void print_mbc_type(gb_cartridge *cart)
{
    const char *mbc_type;

    switch (cart->mbc)
    {
        case UNKNOWN_MBC:
            mbc_type = "Unknown MBC";
            break;

        case NO_MBC:
            mbc_type = "No MBC";
            break;

        case MBC1:
            mbc_type = "MBC1";
            break;

        case MBC2:
            mbc_type = "MBC2";
            break;

        case MBC3:
            mbc_type = "MBC3";
            break;

        case MBC5:
            mbc_type = "MBC5";
            break;

        case MBC6:
            mbc_type = "MBC6";
            break;

        case MBC7:
            mbc_type = "MBC7";
            break;

        case MMM01:
            mbc_type = "MMM01";
            break;

        case HuC1:
            mbc_type = "HuC1";
            break;

        case HuC3:
            mbc_type = "HuC3";
            break;
    }

    printf("MBC Type: %s\n", mbc_type);
}
