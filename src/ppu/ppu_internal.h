#ifndef CBOY_PPU_INTERNAL_H
#define CBOY_PPU_INTERNAL_H

#include "cboy/gameboy.h"
#include "cboy/ppu.h"

/* tile map dimensions (32 tiles = 256 pixels) */
#define TILE_WIDTH 8
#define TILE_MAP_TILE_WIDTH 32
#define TILE_MAP_WIDTH 256

/* sentinel value to indicate the background and
 * window are disabled when rendering scanlines
 */
#define DMG_NO_PALETTE 0x0000


void init_display_colors(display_colors *colors);

void dmg_render_sprite_pixels(gameboy *gb, gb_sprite *sprite);
void dmg_load_bg_tiles(gameboy *gb);
void dmg_load_window_tiles(gameboy *gb);
void dmg_render_scanline(gameboy *gb);
void dmg_push_scanline_data(gameboy *gb);

#endif /* !CBOY_PPU_INTERNAL_H */
