#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "cboy/gameboy.h"
#include "cboy/memory.h"
#include "cboy/cartridge.h"
#include "cboy/interrupts.h"
#include "cboy/instructions.h"

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
    gb->dma_requested = false;
    gb->clock_counter = 0;
    gb->dma_counter = 0;

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

/* Increment the TIMA register, including handling its
 * overflow behavior. When TIMA overflows the value of
 * TMA is loaded and a timer interrupt is requested.
 *
 * TODO: when TIMA overflows, it's value should actually
 * be set to 0x00 for one m-cycle (four CPU clocks) before
 * loading the value of TMA and requesting a timer interrupt.
 */
void increment_tima(gameboy *gb)
{
    uint8_t tma = gb->memory->mmap[TMA_REGISTER],
            incremented_tima = gb->memory->mmap[TIMA_REGISTER] + 1;

    if (!incremented_tima) // TIMA overflowed
    {
        incremented_tima = tma;
        request_interrupt(gb, TIMER);
    }

    gb->memory->mmap[TIMA_REGISTER] = incremented_tima;
}

/* Increment the Game Boy's internal clock counter
 * by the given number of clocks. Also handles
 * updating the DIV and TIMA registers as needed.
 *
 * Because the DIV register is simply the upper
 * byte of this internal counter mapped to memory
 * address 0xff04, incrementing the counter also
 * increments the DIV register as needed (every 256
 * clock cycles).
 *
 * Because the TIMA register (memory address 0xff05)
 * is also a timer, we must increment it as needed
 * when the internal clock counter is incremented. The
 * TIMA register can be enabled/disabled via bit 2 of
 * the TAC register (memory address 0xff07). If enabled,
 * The frequency at which this timer increments is
 * specified by bits 1 and 0 of TAC. These frequencies
 * are specified below:
 *
 * TIMA update frequencies
 * ~~~~~~~~~~~~~~~~~~~~~~~
 * Bits 1 and 0 of TAC:
 *     00: CPU Clock / 1024
 *     01: CPU Clock / 16
 *     10: CPU Clock / 64
 *     11: CPU Clock / 256
 *
 * When TIMA overflows, its value is reset to the value
 * specified in the TMA register (memory address 0xff06)
 * and the Timer Interrupt bit in the IF register is set.
 *
 * NOTE: because this emulation is currently not cycle-accurate,
 * the timings of these increments are not exact. This might
 * cause some bugs for timer-related events in the games
 * being played.
 *
 * TODO: explore ways to improve the timing of these increments
 */
void increment_clock_counter(gameboy *gb, uint16_t num_clocks)
{
    uint8_t tac = gb->memory->mmap[TAC_REGISTER];

    bool tima_enabled = tac & 0x4;

    // the number of CPU clock ticks between TIMA increments
    uint16_t tima_tick_interval;
    switch (tac & 0x3)
    {
        case 0x0:
            tima_tick_interval = 0x400;
            break;
        case 0x1:
            tima_tick_interval = 0x10;
            break;
        case 0x2:
            tima_tick_interval = 0x40;
            break;
        case 0x3:
            tima_tick_interval = 0x100;
            break;
    }

    // increment the internal clock counter one tick at a time
    while (num_clocks)
    {
        ++(gb->clock_counter);
        --num_clocks;

        // check if TIMA needs incrementing
        if (tima_enabled && !(gb->clock_counter % tima_tick_interval))
        {
            increment_tima(gb);
        }
    }
}

// run the emulator
// TODO: add a running flag to gameboy struct
void run_gameboy(gameboy *gb)
{
    printf("Executing 20 intructions from the ROM as a test:\n");

    // simulate a DMA transfer request
    write_byte(gb, 0xff46, 0x00);

    // begin execution (just 20 iterations)
    uint8_t num_clocks;
    for (int i = 0; i < 2000; ++i)
    {
        // number of clock ticks, given number of m-cycles
        num_clocks = 4 * execute_instruction(gb);
        increment_clock_counter(gb, num_clocks);

        /* Check if DMA needs to be performed
         * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
         * To emulate the DMA transfer timing, we wait until the
         * number of clocks that a DMA takes has elapsed since
         * the DMA register was written to, then perform the DMA
         * transfer all at once. This works because the CPU only
         * has access to HRAM while the DMA process is supposed
         * to be occuring.
         *
         * The transfer takes 160 m-cycles (640 clocks)
         */
        if (gb->dma_requested)
        {
            printf("***DMA check***\n");
            gb->dma_counter += num_clocks;
            if (gb->dma_counter >= 640)
            {
                printf("***Performing DMA transfer***\n");
                dma_transfer(gb);
                gb->dma_requested = false;
                gb->dma_counter = 0;
            }
        }

        print_registers(gb->cpu);

        printf("Internal Clock Counter: 0x%04x\n\n", gb->clock_counter);
        // sleep for 0.25 sec
        usleep(250000);
    }

}
