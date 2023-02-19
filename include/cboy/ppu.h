#ifndef GB_PPU_H
#define GB_PPU_H

#include <stdint.h>
#include <stdbool.h>
#include <SDL.h>

#define FRAME_WIDTH  160
#define FRAME_HEIGHT 144

/* PPU-related registers */
#define LCDC_REGISTER 0xff40
#define STAT_REGISTER 0xff41
#define SCY_REGISTER  0xff42
#define SCX_REGISTER  0xff43
#define LY_REGISTER   0xff44
#define LYC_REGISTER  0xff45
#define DMA_REGISTER  0xff46
#define BGP_REGISTER  0xff47
#define OBP0_REGISTER 0xff48
#define OBP1_REGISTER 0xff49
#define WY_REGISTER   0xff4a
#define WX_REGISTER   0xff4b

typedef struct gameboy gameboy;

/* colors for use by the display */
typedef struct display_colors {
    uint32_t white,
             light_gray,
             dark_gray,
             black,
             transparent;

    // whether we're in grayscale or "greenscale" mode
    bool grayscale_mode;
} display_colors;

typedef struct gb_ppu {
    uint32_t frame_buffer[FRAME_WIDTH * FRAME_HEIGHT];
    display_colors colors;
    uint32_t dot_clock;
    uint64_t frames_rendered;
    uint8_t scx, scy, ly, wx, wy;
    bool curr_scanline_rendered, curr_frame_displayed;
} gb_ppu;

void render_scanline(gameboy *gb);

void display_frame(gameboy *gb);

void run_ppu(gameboy *gb, uint8_t num_clocks);

void toggle_display_colors(gb_ppu *ppu);

gb_ppu *init_ppu(void);

void free_ppu(gb_ppu *ppu);

#endif
