#include <stdio.h>
#include "cboy/gameboy.h"
#include "cboy/cpu.h"
#include "cboy/instructions.h"
#include "cboy/cartridge.h"
#include "cboy/memory.h"
#include "cboy/interrupts.h"

int main(int argc, const char *argv[])
{
    printf("CBoy - A Game Boy Emulator\n"
           "--------------------------\n");

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <rom_file_path>\n", argv[0]);
        return 1;
    }

    gameboy *gb = init_gameboy(argv[1]);

    if (gb == NULL)
    {
        return 1;
    }

    printf("Number of banks in the ROM: %d\n"
           "Number of external RAM banks: %d\n"
           "RAM bank size: %d bytes\n"
           "IME flag: %d\n"
           "IE register: 0x%02x\n"
           "IF register: 0x%02x\n\n",
           gb->cart->num_rom_banks,
           gb->cart->num_ram_banks,
           gb->cart->ram_bank_size,
           gb->cpu->ime_flag,
           read_byte(gb, IE_REGISTER),
           read_byte(gb, IF_REGISTER));

    // initial register contents
    print_registers(gb->cpu);

    print_mbc_type(gb->cart);

    if (gb->cart->mbc != NO_MBC)
    {
        fprintf(stderr, "NOTE: MBCs are not yet supported."
                        " This game will not run correctly\n");
    }

    // check if interrupts work
    printf("\nEnabling and requesting all interrupts\n");

    gb->cpu->ime_flag = true;

    enable_interrupt(gb, VBLANK);
    enable_interrupt(gb, LCD_STAT);
    enable_interrupt(gb, TIMER);
    enable_interrupt(gb, SERIAL);
    enable_interrupt(gb, JOYPAD);

    request_interrupt(gb, VBLANK);
    request_interrupt(gb, LCD_STAT);
    request_interrupt(gb, TIMER);
    request_interrupt(gb, SERIAL);
    request_interrupt(gb, JOYPAD);

    printf("IME flag: %d (should be 1)\n"
           "IE register: 0x%02x (should be 0x1f)\n"
           "IF register: 0x%02x (should be 0x1f)\n\n",
           gb->cpu->ime_flag,
           read_byte(gb, IE_REGISTER),
           read_byte(gb, IF_REGISTER));

    // service an interrupt to see if PC is updated correctly
    service_interrupt(gb);

    printf("Servicing interrupt. VBLANK should have been serviced\n"
           "IME flag: %d (should be 0)\n"
           "IE register: 0x%02x (should not change)\n"
           "IF register: 0x%02x (should be 0x1e because VBLANK should have been serviced)\n\n",
           gb->cpu->ime_flag,
           read_byte(gb, IE_REGISTER),
           read_byte(gb, IF_REGISTER));

    printf("PC should be 0x0040 because VBLANK should have been serviced\n");
    print_registers(gb->cpu);

    free_gameboy(gb);
    return 0;
}
