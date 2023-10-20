#ifndef GB_PPU_H
#define GB_PPU_H

#include <stdint.h>
#include <stdbool.h>
#include <SDL.h>

#define FRAME_WIDTH  160
#define FRAME_HEIGHT 144

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
        bool bg_over_obj,
             yflip,
             xflip,
             palette_no;

        // The sprite's tile data. Recall that each tile is
        // 16 bytes in size, so if using 8x8 sprites, the
        // second half of the array is unused.
        uint8_t tile_data[32];
} gb_sprite;

/* colors for use by the display */
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
    uint8_t scx, scy, ly, wx, wy;

    // track palette and color index data for the scanline
    // being rendered so we can mix the background, window,
    // and sprites into a final image
    uint16_t scanline_palette_buff[FRAME_WIDTH];
    uint8_t scanline_coloridx_buff[FRAME_WIDTH];

    // an internal counter that tracks how many lines of
    // the window have been rendered for the current frame
    uint8_t window_line_counter;

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
} gb_ppu;

void render_scanline(gameboy *gb);

void display_frame(gameboy *gb);

void run_ppu(gameboy *gb, uint8_t num_clocks);

void cycle_display_colors(display_colors *colors, bool cycle_forward);

gb_ppu *init_ppu(void);

void free_ppu(gb_ppu *ppu);

void reset_ppu(gameboy *gb);

#endif
