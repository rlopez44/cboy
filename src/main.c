#include <stdio.h>
#include "cboy/gameboy.h"
#include "cboy/cpu.h"
#include "cboy/instructions.h"
#include "cboy/cartridge.h"
#include "cboy/memory.h"

/* Initializes the emulator and loads the
 * ROM file into the emulator.
 *
 * If initialization fails then NULL is
 * returned and an error message is printed
 * out. Otherwise, the initialized gameboy
 * with the ROM loaded is returned.
 */
static gameboy *init_emulator(const char *rom_file_path)
{
    // open the ROM file to load it into the emulator
    FILE *rom_file = fopen(rom_file_path, "rb");

    if (rom_file == NULL)
    {
        fprintf(stderr, "Failed to open the ROM file (incorrect path?)\n");
        return NULL;
    }

    gameboy *gb = init_gameboy();

    if (gb == NULL)
    {
        fprintf(stderr, "Not enough memory to initialize the emulator\n");
        fclose(rom_file);
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

int main(int argc, const char *argv[])
{
    printf("CBoy - A Game Boy Emulator\n"
           "--------------------------\n");

    if (argc != 2)
    {
        fprintf(stderr, "Usage: cboy <rom_file_path>\n");
        return 1;
    }

    // TODO: actually do stuff
    gameboy *gb = init_emulator(argv[1]);

    if (gb == NULL)
    {
        return 1;
    }

    // should be 512, since this is default value during initialization
    printf("Number of banks in the ROM: %d\n", gb->cart->num_banks);

    // should be 1, since the boot ROM is disabled during initialization
    printf("Boot ROM disabled bit: %d\n", read_byte(gb->memory, 0xff50));

    // initial register contents
    print_registers(gb->cpu);

    free_gameboy(gb);
    return 0;
}
