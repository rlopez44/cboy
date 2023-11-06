#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "cboy/common.h"
#include "cboy/gameboy.h"
#include "cboy/cartridge.h"
#include "cboy/mbc.h"
#include "cboy/log.h"

// minimum number of bits needed to store the given value
static uint16_t count_bits(uint16_t n)
{
    uint8_t bit_count = 0;
    while (n)
    {
        ++bit_count;
        n >>= 1;
    }

    return bit_count;
}

/* free the allocated memory for the cartridge struct */
void unload_cartridge(gb_cartridge *cart)
{
    if (!cart)
        return;

    if (cart->rom_banks)
        for (int i = 0; i < cart->num_rom_banks; ++i)
            free(cart->rom_banks[i]);

    free(cart->rom_banks);

    if (cart->ram_banks)
        for (int i = 0; i < cart->num_ram_banks; ++i)
            free(cart->ram_banks[i]);

    free(cart->ram_banks);

    free(cart->mbc);

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
    gb_cartridge *cart = calloc(1, sizeof(gb_cartridge));

    if (cart == NULL)
        return NULL;

    cart->mbc = malloc(sizeof(cartridge_mbc));
    if (cart->mbc == NULL)
    {
        unload_cartridge(cart);
        return NULL;
    }

    return cart;
}

/* determine the number of banks in the ROM given the zeroth ROM bank */
static uint16_t get_num_rom_banks(uint8_t *rom0)
{
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

/* determine the external RAM size */
static int get_ext_ram_size(uint8_t *rom0)
{
    int ram_size;
    switch (rom0[0x149])
    {
        case 0x00:
            ram_size = 0;
            break;
        case 0x01:
            ram_size = 2*KB;
            break;
        case 0x02:
            ram_size = 8*KB;
            break;
        case 0x03:
            ram_size = 32*KB;
            break;
        case 0x04:
            ram_size = 128*KB;
            break;
        case 0x05:
            ram_size = 64*KB;
            break;
        default: // invalid byte
            ram_size = -1;
            break;
    }
    return ram_size;
}

/* determine the number of RAM banks in the cartridge */
static uint16_t get_num_ram_banks(int ext_ram_size)
{
    // special case of one partial RAM bank of size 2 KB (rather than full 8 KB)
    if (ext_ram_size == 2*KB)
        return 1;

    return ext_ram_size / (8 * KB);
}

// Use byte 0x147 of the cartridge header to see if
// the loaded cartridge has a Real Time Clock.
static bool detect_rtc_support(uint8_t *rom0)
{
    bool has_rtc;
    switch (rom0[0x147])
    {
        case 0x0f:
        case 0x10:
            has_rtc = true;
            break;

        default:
            has_rtc = false;
            break;
    }

    return has_rtc;
}

// Allocates banks for ROM and RAM.
static ROM_LOAD_STATUS init_banks(gb_cartridge *cart, int n_rombanks, int n_rambanks)
{
    cart->num_rom_banks = 0;
    cart->num_ram_banks = 0;

    cart->rom_banks = calloc(n_rombanks, sizeof(uint8_t *));
    if (!cart->rom_banks)
        return ROM_LOAD_ERROR;

    for (int i = 0; i < n_rombanks; ++i)
    {
        cart->rom_banks[i] = calloc(ROM_BANK_SIZE, 1);
        if (!cart->rom_banks[i])
            return ROM_LOAD_ERROR;

        ++cart->num_rom_banks;
    }

    cart->ram_banks = NULL;
    if (n_rambanks)
        cart->ram_banks = calloc(n_rambanks, sizeof(uint8_t *));

    // it's possible for a cartridge to have no RAM
    if (!cart->ram_banks && n_rambanks)
        return ROM_LOAD_ERROR;

    for (int i = 0; i < n_rambanks; ++i)
    {
        cart->ram_banks[i] = calloc(cart->ram_bank_size, 1);
        if (!cart->ram_banks[i])
            return ROM_LOAD_ERROR;

        ++cart->num_ram_banks;
    }

    // used by the MBC for ROM/RAM addressing (0..num_banks - 1)
    cart->rom_banks_bitsize = cart->num_rom_banks
                              ? count_bits(cart->num_rom_banks - 1)
                              : 0;
    cart->ram_banks_bitsize = cart->num_ram_banks
                              ? count_bits(cart->num_ram_banks - 1)
                              : 0;

    return ROM_LOAD_SUCCESS;
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
    uint8_t rom_bank_buffer[ROM_BANK_SIZE] = {0};

    /* The first 16 KB (2^14 bytes) are guaranteed
     * to be present in the ROM. The cartridge header
     * is located here and we use its information to
     * finish cartridge initialization.
     */
    ROM_LOAD_STATUS bank_load_status = load_rom_bank(rom_bank_buffer, rom_file);

    if (bank_load_status != ROM_LOAD_SUCCESS)
        return bank_load_status;

    // use the header info to load the rest of the ROM
    int num_rom_banks = get_num_rom_banks(rom_bank_buffer);
    int ext_ram_size = get_ext_ram_size(rom_bank_buffer);
    cart->mbc_type = get_mbc_type(rom_bank_buffer);
    cart->has_rtc = detect_rtc_support(rom_bank_buffer);

    if (!num_rom_banks || ext_ram_size == -1 || cart->mbc_type == UNKNOWN_MBC)
        return MALFORMED_ROM;

    init_mbc(cart->mbc_type, cart->mbc);
    cart->ram_bank_size = ext_ram_size;

    bank_load_status = init_banks(cart, num_rom_banks, get_num_ram_banks(ext_ram_size));

    if (bank_load_status != ROM_LOAD_SUCCESS)
        return bank_load_status;

    memcpy(cart->rom_banks[0], rom_bank_buffer, ROM_BANK_SIZE);

    for (int i = 1; i < cart->num_rom_banks; ++i)
    {
        bank_load_status = load_rom_bank(rom_bank_buffer, rom_file);

        // I/O error or malformed ROM
        if (bank_load_status != ROM_LOAD_SUCCESS)
            return bank_load_status;

        memcpy(cart->rom_banks[i], rom_bank_buffer, ROM_BANK_SIZE);
    }

    return ROM_LOAD_SUCCESS;
}

/* print the ROM's title */
void print_rom_title(gb_cartridge *cart)
{
    // the title is max 16 characters (17 w/null terminator)
    // and is located at address 0x0134 in the first ROM bank
    char title[17] = {0};
    memcpy(title, cart->rom_banks[0] + 0x0134, 16);
    LOG_INFO("Title: %s\n", title);
}

static char *get_ramsav_filename(const char *romfile)
{
    const char *ext = "cboysav";
    const size_t extlen = strlen(ext);
    const size_t rom_pathlen = strlen(romfile);
    char *savepath = calloc(rom_pathlen + extlen + 1, 1);

    if (savepath == NULL)
        return NULL;

    strcpy(savepath, romfile);
    strcpy(savepath + rom_pathlen, ext);

    return savepath;
}

void maybe_import_cartridge_ram(gb_cartridge *cart, const char *romfile)
{
    if (!cart->num_ram_banks && !cart->has_rtc)
        return;

    char *filepath = get_ramsav_filename(romfile);
    if (filepath == NULL)
        goto mem_error;

    FILE *ramfile = fopen(filepath, "rb");
    if (ramfile == NULL)
        goto fopen_error;

    size_t bytes_read;
    for (int i = 0; i < cart->num_ram_banks; ++i)
    {
        bytes_read = fread(cart->ram_banks[i],
                           1,
                           cart->ram_bank_size,
                           ramfile);

        if (bytes_read != cart->ram_bank_size)
            goto read_error;
    }

    // see `save_cartridge_ram()` for format info
    if (cart->has_rtc)
    {
        cartridge_mbc3 *mbc = &cart->mbc->mbc3;
        uint64_t snapshot_time = 0;
        uint8_t rtc_data[48];

        bytes_read = fread(rtc_data, 1, sizeof rtc_data, ramfile);

        if (bytes_read != sizeof rtc_data)
            goto read_error;

        // internal RTC registers
        mbc->rtc_s = rtc_data[0];
        mbc->rtc_m = rtc_data[4];
        mbc->rtc_h = rtc_data[8];
        mbc->rtc_d = rtc_data[12];
        mbc->rtc_d |= (rtc_data[16] & 1) << 8;
        mbc->day_carry = (rtc_data[16] >> 7) & 1;
        mbc->rtc_halt = (rtc_data[16] >> 6) & 1;

        // latched RTC registers
        for (uint8_t i = 0; i < 5; ++i)
            mbc->rtc_latched_values[i] = rtc_data[20 + 4*i];

        // timestamp
        for (uint8_t i = 0; i < 8; ++i)
            snapshot_time |= rtc_data[40 + i] << (8 * i);

        // tick the RTC registers to get them up to date
        uint64_t seconds_elapsed = (uint64_t)time(NULL) - snapshot_time;
        fast_forward_rtc(mbc, seconds_elapsed);
    }

    fclose(ramfile);
    free(filepath);
    return;

read_error:
    fclose(ramfile);
fopen_error:
    free(filepath);
mem_error:
    for (int i = 0; i < cart->num_ram_banks; ++i)
        memset(cart->ram_banks[i], 0, cart->ram_bank_size);

    // we didn't fully read the RTC data, so reset the MBC
    if (cart->has_rtc)
        init_mbc(cart->mbc_type, cart->mbc);
}

void save_cartridge_ram(gb_cartridge *cart, const char *romfile)
{
    if (!cart->num_ram_banks && !cart->has_rtc)
        return;

    char *savepath = get_ramsav_filename(romfile);
    if (savepath == NULL)
        goto mem_error;

    FILE *savefile = fopen(savepath, "wb");
    if (savefile == NULL)
        goto fopen_error;

    size_t bytes_written;
    for (int i = 0; i < cart->num_ram_banks; ++i)
    {
        bytes_written = fwrite(cart->ram_banks[i],
                               1,
                               cart->ram_bank_size,
                               savefile);

        if (bytes_written != cart->ram_bank_size)
            goto write_error;
    }

    // If the cartridge has an RTC, we append RTC info to the save
    // file following: https://bgb.bircd.org/rtcsave.html.
    // The registers are one byte in size, but are stored in the save file
    // as 4 byte little endian data with appropriate zero padding.
    if (cart->has_rtc)
    {
        cartridge_mbc3 *mbc = &cart->mbc->mbc3;
        uint64_t curr_time = time(NULL);
        uint8_t rtc_data[48] = {0};

        // internal RTC registers
        rtc_data[0]  = mbc->rtc_s;
        rtc_data[4]  = mbc->rtc_m;
        rtc_data[8]  = mbc->rtc_h;
        rtc_data[12] = mbc->rtc_d & 0xff;
        rtc_data[16] = mbc->day_carry << 7
                       | mbc->rtc_halt << 6
                       | ((mbc->rtc_d >> 8) & 1);

        // latched RTC registers
        for (uint8_t i = 0; i < 5; ++i)
            rtc_data[20 + 4*i] = mbc->rtc_latched_values[i];

        // timestamp
        for (uint8_t i = 0; i < 8; ++i)
        {
            rtc_data[40 + i] = curr_time & 0xff;
            curr_time >>= 8;
        }
        bytes_written = fwrite(rtc_data, 1, sizeof rtc_data, savefile);

        if (bytes_written != sizeof rtc_data)
            goto write_error;
    }

    free(savepath);
    fclose(savefile);
    return;

write_error:
    fclose(savefile);
    remove(savepath);
fopen_error:
    free(savepath);
mem_error:
    LOG_ERROR("\nCould not save cartridge RAM (memory or I/O error).\n");
}
