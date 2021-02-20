#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
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
    write_byte(gb->memory, --(gb->cpu->reg->sp), (uint8_t)(value >> 8));
    write_byte(gb->memory, --(gb->cpu->reg->sp), (uint8_t)(value & 0xff));
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
    uint8_t lo = read_byte(gb->memory, (gb->cpu->reg->sp)++);
    uint8_t hi = read_byte(gb->memory, (gb->cpu->reg->sp)++);

    return (uint16_t)(hi << 8) | (uint16_t)lo;
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

    // allocate and init the memory map
    gb->memory = init_memory_map();

    if (gb->memory == NULL)
    {
        free_cpu(gb->cpu);
        free(gb);
        return NULL;
    }

    // Allocate and init the cartridge
    gb->cart = init_cartridge();

    if (gb->cart == NULL)
    {
        free_memory_map(gb->memory);
        free_cpu(gb->cpu);
        free(gb);
        return NULL;
    }

    // open the ROM file to load it into the emulator
    FILE *rom_file = fopen(rom_file_path, "rb");

    if (rom_file == NULL)
    {
        fprintf(stderr, "Failed to open the ROM file (incorrect path?)\n");
        free_gameboy(gb);
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

        free_gameboy(gb);
        return NULL;
    }

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
