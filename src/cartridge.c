#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "cboy/cartridge.h"

/* 16 KB (2^14 bytes) */
#define ROM_BANK_SIZE 16384

#define MAX_ROM_BANKS 512

/* free the allocated memory for the cartridge struct */
void unload_cartridge(gb_cartridge *cart)
{
    // free the ROM banks
    for (int i = 0; i < cart->num_banks; ++i)
    {
        free((cart->rom_banks)[i]);
    }

    // free the ROM banks array
    free(cart->rom_banks);

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

    // allocate the ROM banks array
    cart->rom_banks = malloc(MAX_ROM_BANKS * sizeof(uint8_t *));

    // ROM banks array allocation failed
    if (cart->rom_banks == NULL)
    {
        free(cart);
        return NULL;
    }

    // allocate the ROM banks and initialize values to zero
    int num_alloced_banks = 0;
    for (int i = 0; i < MAX_ROM_BANKS; ++i)
    {
        (cart->rom_banks)[i] = calloc(ROM_BANK_SIZE, sizeof(uint8_t));

        // ROM bank allocation failed
        if ((cart->rom_banks)[i] == NULL)
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
            free((cart->rom_banks)[i]);
        }

        free(cart->rom_banks);
        free(cart);
        return NULL;
    }

    // all allocations were successful
    cart->num_banks = MAX_ROM_BANKS;
    return cart;
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

    // Use the header info to load the rest of the ROM.
    // Byte 0x148 tells us the total ROM size.
    uint16_t num_banks;
    switch (rom_bank_buffer[0x148])
    {
        case 0x00:
            num_banks = 2;
            break;
        case 0x01:
            num_banks = 4;
            break;
        case 0x02:
            num_banks = 8;
            break;
        case 0x03:
            num_banks = 16;
            break;
        case 0x04:
            num_banks = 32;
            break;
        case 0x05:
            num_banks = 64;
            break;
        case 0x06:
            num_banks = 128;
            break;
        case 0x07:
            num_banks = 256;
            break;
        case 0x08:
            num_banks = 512;
            break;
        case 0x52:
            num_banks = 72;
            break;
        case 0x53:
            num_banks = 80;
            break;
        case 0x54:
            num_banks = 96;
            break;
        default: // not a valid value at this byte
            num_banks = 0;
            break;
    }

    // invalid ROM size byte
    if (!num_banks)
    {
        return MALFORMED_ROM;
    }

    // copy ROM bank 0 from buffer to the actual bank
    memcpy((cart->rom_banks)[0], rom_bank_buffer, ROM_BANK_SIZE * sizeof(uint8_t));

    // copy the remaining ROM banks
    for (int i = 1; i < num_banks; ++i)
    {
        bank_load_status = load_rom_bank(rom_bank_buffer, rom_file);

        // I/O error or malformed ROM
        if (bank_load_status != ROM_LOAD_SUCCESS)
        {
            return bank_load_status;
        }

        // bank copied successfully, so move it out of the buffer
        memcpy((cart->rom_banks)[i], rom_bank_buffer, ROM_BANK_SIZE * sizeof(uint8_t));
    }


    /* All banks copied successfully, so resize ROM banks array if needed.
     * Since the cartridge is initialized with the maximum number of ROM
     * banks, we only need to shrink down the ROM banks array.
     */
    if (num_banks != MAX_ROM_BANKS)
    {
        // free the unused banks before resizing the banks array
        for (int i = num_banks; i < MAX_ROM_BANKS; ++i)
        {
            free((cart->rom_banks)[i]);
        }

        uint8_t **tmp = realloc(cart->rom_banks, num_banks * sizeof(uint8_t *));

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
        cart->num_banks = num_banks;
    }

    return ROM_LOAD_SUCCESS;
}
