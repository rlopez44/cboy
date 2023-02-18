#ifndef GAME_BOY_H
#define GAME_BOY_H

#include <stdbool.h>
#include <stdint.h>
#include "cboy/cpu.h"
#include "cboy/memory.h"
#include "cboy/cartridge.h"
#include "cboy/ppu.h"
#include "cboy/joypad.h"

#define JOYP_REGISTER 0xff00

#define DIV_REGISTER 0xff04
#define TIMA_REGISTER 0xff05
#define TMA_REGISTER 0xff06
#define TAC_REGISTER 0xff07

/* frame duration is 16.74 ms */
#define GB_FRAME_DURATION_MS 17

/* boot ROM size in bytes */
#define BOOT_ROM_SIZE 256

typedef struct gameboy {
    gb_cpu *cpu;
    gb_memory *memory;
    gb_cartridge *cart;
    gb_ppu *ppu;
    gb_joypad *joypad;

    // if the Game Boy is still on
    bool is_on;

    // Game Boy boot ROM, if passed into the emulator
    uint8_t boot_rom[BOOT_ROM_SIZE];
    bool run_boot_rom;

    bool is_stopped, dma_requested;

    // to maintain the appropriate frame rate
    uint32_t next_frame_time;

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

    // our Game Boy screen
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *screen;
} gameboy;

// stack push and pop operations
void stack_push(gameboy *gb, uint16_t value);

uint16_t stack_pop(gameboy *gb);

// initialize the Game Boy
gameboy *init_gameboy(const char *rom_file_path, const char *bootrom);

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
#endif /* GAME_BOY_H */
