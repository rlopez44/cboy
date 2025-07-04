#ifndef GB_PPU_H
#define GB_PPU_H

#include <stdint.h>
#include <stdbool.h>
#include <SDL3/SDL.h>
#include "cboy/common.h"

#define FRAME_WIDTH  160
#define FRAME_HEIGHT 144

/* palette/color RAM */
#define PRAM_SIZE 64

typedef struct gameboy gameboy;

/* Sprite rendering data */
typedef struct gb_sprite {
        uint8_t ypos, // sprite vertical pos + 16
                xpos, // sprite horizontal pos + 8
                tile_idx,
                ysize;

        // needed for drawing priority
        uint8_t oam_offset;

        // sprite attributes
        bool bg_over_obj, // object priority
             yflip,       // vertical mirror
             xflip;       // horizontal mirror

        // selects OBP0-1 (DMG) or OBP0-7 (CGB)
        uint8_t palette_no;

        // select VRAM bank 0 or 1 (CGB only)
        bool vram_bank;

        // The sprite's tile data. Recall that each tile is
        // 16 bytes in size, so if using 8x8 sprites, the
        // second half of the array is unused.
        uint8_t tile_data[32];
} gb_sprite;

/* colors for use by the display in monochrome mode */
typedef struct display_colors {
    uint16_t white,
             light_gray,
             dark_gray,
             black;

    uint8_t palette_index;
} display_colors;

typedef struct gb_ppu {
    uint16_t frame_buffer[FRAME_WIDTH * FRAME_HEIGHT];
    display_colors colors;
    uint32_t dot_clock;
    uint64_t frames_rendered;

    // background/window palette (color) RAM
    uint8_t bg_pram[PRAM_SIZE];

    // object palette (color) RAM
    uint8_t obj_pram[PRAM_SIZE];

    // an internal counter that tracks how many lines of
    // the window have been rendered for the current frame
    uint8_t window_line_counter;

    // Latched when LY first equals WY in a given frame. Reset on VBlank.
    // The window is only eligible to be drawn after the trigger is latched.
    bool wy_trigger;

    bool curr_scanline_rendered, curr_frame_displayed;

    /* The mode 0-2 and LY=LYC STAT mode interrupt
     * sources are ORed together for purposes
     * of requesting STAT interrupts. This
     * prevents multiple consecutive STAT
     * interrupt requests from these sources.
     */
    bool lyc_stat_line,
         hblank_stat_line,
         vblank_stat_line,
         oam_stat_line;

    // the PPU I/O registers
    uint8_t lcdc, stat, scy, scx, ly, lyc;
    uint8_t dma, bgp, obp0, obp1,  wx, wy;

    // CGB PPU I/O registers
    uint8_t bcps, ocps, opri;

    // CGB only: whether to apply correction to emulate LCD color output
    bool lcd_filter;
} gb_ppu;

void ppu_write(gameboy *gb, uint16_t address, uint8_t value);

uint8_t ppu_read(gameboy *gb, uint16_t address);

void dma_transfer(gameboy *gb);

void display_frame(gameboy *gb);

uint16_t tile_addr_from_index(bool tile_data_area_bit, uint8_t tile_index);

void load_sprites(gameboy *gb);

void run_ppu(gameboy *gb, uint8_t num_clocks);

void cycle_display_colors(display_colors *colors, bool cycle_forward);

gb_ppu *init_ppu(enum GAMEBOY_MODE gb_mode);

void free_ppu(gb_ppu *ppu);

void reset_ppu(gameboy *gb);

#endif
