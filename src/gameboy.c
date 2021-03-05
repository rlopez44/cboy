#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "cboy/gameboy.h"
#include "cboy/memory.h"
#include "cboy/cartridge.h"

// Stack pop and push operations
void stack_push(gameboy *gb, uint16_t value)
{
    /* NOTE: The stack grows downward (decreasing address).
     *
     * The push operation can be thought of as performing
     * the following imaginary instructions:
     *
     *  DEC SP
     *  LD [SP], HIGH_BYTE(value) ; little-endian, so hi byte first
     *  DEC SP
     *  LD [SP], LOW_BYTE(value)
     */
    write_byte(gb, --(gb->cpu->reg->sp), (uint8_t)(value >> 8));
    write_byte(gb, --(gb->cpu->reg->sp), (uint8_t)(value & 0xff));
}

uint16_t stack_pop(gameboy *gb)
{
    /* The pop operation can be thought of as performing
     * the inverse of the push's imaginary operations:
     *
     *  LD LOW_BYTE(value), [SP] ; little-endian
     *  INC SP
     *  LD HIGH_BYTE(value), [SP]
     *  INC SP
     */
    uint8_t lo = read_byte(gb, (gb->cpu->reg->sp)++);
    uint8_t hi = read_byte(gb, (gb->cpu->reg->sp)++);

    return (uint16_t)(hi << 8) | (uint16_t)lo;
}

/* Verifies the Nintendo logo bitmap located in the
 * ROM file. If this bitmap is incorrect, an error
 * is printed out indicating this game wouldn't
 * run on an actual Game Boy. However, the emulator
 * doesn't actually care if the bitmap is correct.
 */
static bool verify_logo(gameboy *gb)
{
    // The correct bytes for the Game Boy logo
    // See: https://gbdev.io/pandocs/#the-cartridge-header
    const uint8_t nintendo_logo[48] = {
        0xce, 0xed, 0x66, 0x66, 0xcc, 0x0d, 0x00, 0x0b,
        0x03, 0x73, 0x00, 0x83, 0x00, 0x0c, 0x00, 0x0d,
        0x00, 0x08, 0x11, 0x1f, 0x88, 0x89, 0x00, 0x0e,
        0xdc, 0xcc, 0x6e, 0xe6, 0xdd, 0xdd, 0xd9, 0x99,
        0xbb, 0xbb, 0x67, 0x63, 0x6e, 0x0e, 0xec, 0xcc,
        0xdd, 0xdc, 0x99, 0x9f, 0xbb, 0xb9, 0x33, 0x3e,
    };

    // The logo bitmap is located at bytes 0x104-0x133 in the ROM
    uint8_t rom_nintendo_logo[48];
    uint8_t *logo_addr = gb->cart->rom_banks[0] + 0x104;
    memcpy(rom_nintendo_logo, logo_addr, sizeof rom_nintendo_logo);

    // check the bitmap
    bool valid_bitmap = !memcmp(nintendo_logo,
                                rom_nintendo_logo,
                                sizeof nintendo_logo);

    if (!valid_bitmap)
    {
        fprintf(stderr, "NOTE: The ROM Nintendo logo bitmap is incorrect."
                        " This ROM wouldn't run on a real Game Boy\n\n");
    }

    return valid_bitmap;
}

/* Verifies the cartridge header checksum in
 * the ROM file. Prints out an error if the
 * checksum fails. However, the emulator
 * doesn't actually care if the checksum fails.
 */
static bool verify_checksum(gameboy *gb)
{
    // header checksum is at byte 0x14d of the zeroth ROM bank
    uint8_t *rom0 = gb->cart->rom_banks[0],
            header_checksum = rom0[0x14d];

    // calculate checksum of bytes at addresses 0x134-0x14c
    // See: https://gbdev.io/pandocs/#the-cartridge-header
    int calculated_checksum = 0;
    for (int i = 0x134; i <= 0x14c; ++i)
    {
        calculated_checksum -= rom0[i] + 1;
    }
    // lower byte of the result is compared to the checksum
    calculated_checksum &= 0xff;

    bool valid_checksum = calculated_checksum == header_checksum;

    if (!valid_checksum)
    {
        fprintf(stderr, "NOTE: The ROM header checksum failed.\n"
                        "Expected: %d\n"
                        "Actual: %d\n"
                        "This ROM wouldn't run on a real Game Boy\n\n",
                        header_checksum,
                        calculated_checksum);
    }

    return valid_checksum;
}

/* Allocates memory for the Game Boy struct
 * and initializes the Game Boy and its components.
 * Loads the ROM file into the emulator.
 *
 * If initialization fails then NULL is returned
 * and an error message is printed out.
 */
gameboy *init_gameboy(const char *rom_file_path)
{
    // allocate the Game Boy struct
    gameboy *gb = malloc(sizeof(gameboy));

    if (gb == NULL)
    {
        fprintf(stderr, "Not enough memory to initialize the emulator\n");
        return NULL;
    }
    gb->is_halted = false;
    gb->is_stopped = false;

    // allocate and init the CPU
    gb->cpu = init_cpu();

    if (gb->cpu == NULL)
    {
        free(gb);
        return NULL;
    }

    // allocate and init the cartridge
    gb->cart = init_cartridge();

    if (gb->cart == NULL)
    {
        free_cpu(gb->cpu);
        free(gb);
        return NULL;
    }

    // open the ROM file to load it into the emulator
    FILE *rom_file = fopen(rom_file_path, "rb");

    if (rom_file == NULL)
    {
        fprintf(stderr, "Failed to open the ROM file (incorrect path?)\n");
        unload_cartridge(gb->cart);
        free_cpu(gb->cpu);
        free(gb);
        return NULL;
    }

    // load the ROM file into the emulator
    ROM_LOAD_STATUS load_status = load_rom(gb->cart, rom_file);
    fclose(rom_file);

    if (load_status != ROM_LOAD_SUCCESS)
    {
        if (load_status == MALFORMED_ROM)
        {
            fprintf(stderr, "ROM file is incorrectly formatted\n");
        }
        else if (load_status == ROM_LOAD_ERROR)
        {
            fprintf(stderr, "Failed to load the ROM into the emulator (I/O or memory error)\n");
        }

        unload_cartridge(gb->cart);
        free_cpu(gb->cpu);
        free(gb);
        return NULL;
    }

    // allocate and init the memory map
    gb->memory = init_memory_map(gb->cart);

    if (gb->memory == NULL)
    {
        unload_cartridge(gb->cart);
        free_cpu(gb->cpu);
        free(gb);
        return NULL;
    }

    // verify the ROM's Nintendo logo bitmap
    verify_logo(gb);

    // verify the ROM's header checksum
    verify_checksum(gb);

    // init successful and ROM loaded
    return gb;
}

/* Free the allocated memory for the Game Boy struct */
void free_gameboy(gameboy *gb)
{
    free_memory_map(gb->memory);
    free_cpu(gb->cpu);
    unload_cartridge(gb->cart);
    free(gb);
}
