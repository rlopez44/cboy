#include <stdio.h>
#include "cboy/gameboy.h"
#include "cboy/cpu.h"
#include "cboy/instructions.h"
#include "cboy/cartridge.h"
#include "cboy/memory.h"

int main(int argc, const char *argv[])
{
    if (argc != 2)
    {
        printf("CBoy - A Game Boy Emulator\nUsage: cboy <rom_file_path>\n");
    }
    else // TODO: actually do stuff
    {
        gameboy *gb = init_gameboy();

        if (gb == NULL)
        {
            printf("Failed to initialize the emulator\n");
            return 1;
        }
        printf("CBoy - A Game Boy Emulator\n");

        // should be 512, since this is default value during initialization
        printf("Number of banks in the ROM: %d\n", gb->cart->num_banks);

        // should be 1, since the boot ROM is disabled during initialization
        printf("Boot ROM disabled bit: %d\n", read_byte(gb->memory, 0xff50));

        // initial register contents
        print_registers(gb->cpu);

        free_gameboy(gb);
    }
    return 0;
}
