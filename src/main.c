#include <stdio.h>
#include <unistd.h>
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

    // initial register contents
    print_registers(gb->cpu);

    printf("IF register: 0x%02x\n\n",
           read_byte(gb, IF_REGISTER));

    if (gb->cart->mbc != NO_MBC)
    {
        fprintf(stderr, "NOTE: MBCs are not yet supported."
                        " This game will not run correctly\n");
    }

    printf("Executing 20 intructions from the ROM as a test:\n");

    // enable TIMA and set it to increment every 64 CPU clocks
    write_byte(gb, TAC_REGISTER, 0x06);

    // set TMA to 0x0a so that we reset to this value when TIMA overflows
    write_byte(gb, TMA_REGISTER, 0x0a);

    // begin execution (just 20 iterations)
    uint8_t num_clocks;
    for (int i = 0; i < 20; ++i)
    {
        // number of clock ticks, given number of m-cycles
        num_clocks = 4 * execute_instruction(gb);
        increment_clock_counter(gb, num_clocks);

        print_registers(gb->cpu);

        printf("IF register: 0x%02x\n"
               "TIMA register: 0x%02x\n"
               "TMA register: 0x%02x\n"
               "TAC register: 0x%02x\n\n",
               read_byte(gb, IF_REGISTER),
               read_byte(gb, TIMA_REGISTER),
               read_byte(gb, TMA_REGISTER),
               read_byte(gb, TAC_REGISTER));

        printf("Internal Clock Counter: 0x%04x\n\n", gb->clock_counter);
        // sleep for 0.25 sec
        usleep(250000);
    }

    // check if writing to DIV works
    printf("Checking if writing to DIV works:\n");
    write_byte(gb, DIV_REGISTER, 0xff);
    printf("Internal Clock Counter: 0x%04x (should be 0)\n\n", gb->clock_counter);

    // check if writing to TIMA works
    printf("Checking if writing to TIMA works (writing 0x10):\n");
    write_byte(gb, TIMA_REGISTER, 0x10);
    printf("TIMA register: 0x%02x (should be 0x10)\n\n", read_byte(gb, TIMA_REGISTER));

    // check if writing to TMA works
    printf("Checking if writing to TMA works (writing 0x10):\n");
    write_byte(gb, TMA_REGISTER, 0x10);
    printf("TMA register: 0x%02x (should be 0x10)\n\n", read_byte(gb, TMA_REGISTER));

    // check if writing to TAC works
    printf("Checking if writing to TAC works (writing 0x1e).\n"
           " Note, because only the lower three bits of TAC\n"
           " can be written to, the resulting value should\n"
           " be 0x1e & 0x07 = 0x06\n");
    write_byte(gb, TAC_REGISTER, 0x1e);
    printf("TAC register: 0x%02x (should be 0x06)\n\n", read_byte(gb, TAC_REGISTER));




    // check if writes to DIV and TAC cause TIMA to increment when conditions are right
    printf("Testing conditions that cause TIMA to increment when writing to DIV or TAC:\n");

    const char *register_msg = "Internal clock counter: 0x%04x\nDIV register: 0x%02x\n"
                               "TIMA register: 0x%02x\nTAC register: 0x%02x\n\n";

    gb->clock_counter = 0xfff7; // every bit except bit 3 is 1
    // access the memory map directly to avoid triggering the TIMA increment
    gb->memory->mmap[TIMA_REGISTER] = 0x00;
    gb->memory->mmap[TAC_REGISTER] = 0x06;

    printf(register_msg,
           gb->clock_counter,
           read_byte(gb, DIV_REGISTER),
           read_byte(gb, TIMA_REGISTER),
           read_byte(gb, TAC_REGISTER));

    printf("Changing TIMA frequency to CPU clock / 16. TIMA should increment\n"
           "because bit 3 of the internal clock counter will be selected and\n"
           "is 0, whereas the currently-selected bit is 1 (all bits except\n"
           "bit 3 are 1)\n");
    write_byte(gb, TAC_REGISTER, 0x05);
    printf(register_msg,
           gb->clock_counter,
           read_byte(gb, DIV_REGISTER),
           read_byte(gb, TIMA_REGISTER),
           read_byte(gb, TAC_REGISTER));

    printf("Changing TIMA frequency to CPU clock / 1024. Nothing should happen to TIMA\n"
           "because we're switching from a selected bit that is 0 to one that is 1\n");
    write_byte(gb, TAC_REGISTER, 0x04);
    printf(register_msg,
           gb->clock_counter,
           read_byte(gb, DIV_REGISTER),
           read_byte(gb, TIMA_REGISTER),
           read_byte(gb, TAC_REGISTER));

    printf("TIMA is currently enabled. Disabling TIMA by writing to TAC. TIMA\n"
           "should increment, because the currently-selected bit is 1 and the\n"
           "enable bit of TAC will flip from 1 to 0\n");
    write_byte(gb, TAC_REGISTER, 0x01);
    printf(register_msg,
           gb->clock_counter,
           read_byte(gb, DIV_REGISTER),
           read_byte(gb, TIMA_REGISTER),
           read_byte(gb, TAC_REGISTER));

    printf("Enabling TIMA by writing to TAC. Nothing should happen to TIMA\n");
    write_byte(gb, TAC_REGISTER, 0x04);
    printf(register_msg,
           gb->clock_counter,
           read_byte(gb, DIV_REGISTER),
           read_byte(gb, TIMA_REGISTER),
           read_byte(gb, TAC_REGISTER));

    printf("Writing to DIV. DIV and the internal clock counter should reset.\n"
           "This will cause the currently-selected bit to flip from 1 to 0.\n"
           "Because TIMA is enabled when this occurs, it will increment as a result\n");
    write_byte(gb, DIV_REGISTER, 0xae);
    printf(register_msg,
           gb->clock_counter,
           read_byte(gb, DIV_REGISTER),
           read_byte(gb, TIMA_REGISTER),
           read_byte(gb, TAC_REGISTER));

    free_gameboy(gb);
    return 0;
}
