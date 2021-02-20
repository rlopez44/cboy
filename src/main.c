#include <stdio.h>
#include "cboy/gameboy.h"
#include "cboy/cpu.h"
#include "cboy/instructions.h"
#include "cboy/cartridge.h"
#include "cboy/memory.h"

int main(int argc, const char *argv[])
{
    printf("CBoy - A Game Boy Emulator\n"
           "--------------------------\n");

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <rom_file_path>\n", argv[0]);
        return 1;
    }

    // TODO: actually do stuff
    gameboy *gb = init_gameboy(argv[1]);

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
