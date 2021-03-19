#ifndef GAME_BOY_H
#define GAME_BOY_H

#include <stdbool.h>
#include <stdint.h>
#include "cboy/cpu.h"
#include "cboy/memory.h"
#include "cboy/cartridge.h"

#define DIV_REGISTER 0xff04
#define TIMA_REGISTER 0xff05
#define TMA_REGISTER 0xff06
#define TAC_REGISTER 0xff07

typedef struct gameboy {
    gb_cpu *cpu;
    gb_memory *memory;
    gb_cartridge *cart;

    bool is_stopped, is_halted, dma_requested;

    /* The Game Boy's internal 16-bit clock counter.
     * The DIV register at memory address 0xff04 is
     * really the upper byte of this counter mapped
     * to memory.
     */
    uint16_t clock_counter;

    /* A counter to track the number of clocks since
     * a DMA transfer was requested so that we can
     * emulate the DMA transfer timing.
     */
    uint16_t dma_counter;
} gameboy;

// stack push and pop operations
void stack_push(gameboy *gb, uint16_t value);

uint16_t stack_pop(gameboy *gb);

// initialize the Game Boy
gameboy *init_gameboy(const char *rom_file_path);

// free the Game Boy
void free_gameboy(gameboy *gb);

// inrement TIMA and handle its overflow behavior
void increment_tima(gameboy *gb);

/* Increment the Game Boy's internal clock counter
 * by the given number of clocks. Also handles
 * incrementing the DIV and TIMA registers as needed.
 */
void increment_clock_counter(gameboy *gb, uint16_t num_clocks);

// the emulator's game loop
void run_gameboy(gameboy *gb);

#endif
