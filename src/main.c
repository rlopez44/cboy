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

    printf("Number of banks in the ROM: %d\n"
           "Number of external RAM banks: %d\n"
           "RAM bank size: %d bytes\n"
           "Boot ROM disabled bit: %d\n\n",
           gb->cart->num_rom_banks,
           gb->cart->num_ram_banks,
           gb->cart->ram_bank_size,
           read_byte(gb->memory, 0xff50));

    // initial register contents
    print_registers(gb->cpu);

    print_mbc_type(gb->cart);

    free_gameboy(gb);
    return 0;
}
