#ifndef GAME_BOY_H
#define GAME_BOY_H

#include <stdbool.h>
#include <stdint.h>
#include "cboy/common.h"
#include "cboy/cpu.h"
#include "cboy/memory.h"
#include "cboy/cartridge.h"
#include "cboy/ppu.h"
#include "cboy/joypad.h"
#include "cboy/apu.h"

#define DMG_BOOT_ROM_SIZE  256 /* bytes */
#define CGB_BOOT_ROM_SIZE 2304 /* bytes */

/* 4 seems like a good default */
#define DEFAULT_WINDOW_SCALE 4

struct gb_init_args {
    char *bootrom;
    char *romfile;
    bool force_dmg;
    int window_scale;
};

typedef struct gameboy {
    gb_cpu *cpu;
    gb_memory *memory;
    gb_cartridge *cart;
    gb_ppu *ppu;
    gb_joypad *joypad;
    gb_apu *apu;

    // if the Game Boy is still on
    bool is_on;

    enum GAMEBOY_MODE run_mode;

    uint8_t boot_rom[CGB_BOOT_ROM_SIZE]; // big enough for DMG and CGB
    bool run_boot_rom;
    bool boot_rom_disabled;

    bool is_stopped, dma_requested;

    // so we can poll input once per frame
    bool frame_presented_signal;

    // we use sync-to-audio to maintain appropriate emulation speed
    bool audio_sync_signal;

    bool throttle_fps;

    /* The Game Boy's internal 16-bit clock counter.
     * The DIV register at memory address 0xff04 is
     * really the upper byte of this counter mapped
     * to memory.
     */
    uint16_t clock_counter;

    // timer counter, modulo, and control registers
    uint8_t tima, tma, tac;

    uint8_t key0; // GB compatibility
    uint8_t vbk;  // VRAM bank
    uint8_t svbk; // WRAM bank

    bool double_speed;
    bool speed_switch_armed;

    // VRAM DMA
    uint16_t vram_dma_source;
    uint16_t vram_dma_dest;
    uint16_t vram_dma_length;
    bool gdma_running;
    bool hdma_running;
    bool hblank_signal; // to time HDMA transfers

    /* A counter to track the number of clocks since
     * a DMA transfer was requested so that we can
     * emulate the DMA transfer timing.
     */
    uint16_t dma_counter;

    uint8_t volume_slider;

    // our Game Boy screen
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *screen;
} gameboy;

// stack push and pop operations
void stack_push(gameboy *gb, uint16_t value);

uint16_t stack_pop(gameboy *gb);

// initialize the Game Boy
gameboy *init_gameboy(struct gb_init_args *args);

// free the Game Boy
void free_gameboy(gameboy *gb);

// inrement TIMA and handle its overflow behavior
void increment_tima(gameboy *gb);

/* Increment the Game Boy's internal clock counter
 * by the given number of clocks. Also handles
 * incrementing the DIV and TIMA registers as needed.
 */
void increment_clock_counter(gameboy *gb, uint16_t num_clocks);

void timing_related_write(gameboy *gb, uint16_t address, uint8_t value);
uint8_t timing_related_read(gameboy *gb, uint16_t address);

void cgb_core_io_write(gameboy *gb, uint16_t address, uint8_t value);
uint8_t cgb_core_io_read(gameboy *gb, uint16_t address);

bool maybe_switch_speed(gameboy *gb);

// the emulator's game loop
void run_gameboy(gameboy *gb);

void report_volume_level(gameboy *gb, bool add_newline);

#endif /* GAME_BOY_H */
