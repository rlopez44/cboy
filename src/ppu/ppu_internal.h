#ifndef CBOY_PPU_INTERNAL_H
#define CBOY_PPU_INTERNAL_H

#include <stdint.h>
#include "cboy/gameboy.h"
#include "cboy/ppu.h"

/* tile map dimensions (32 tiles = 256 pixels) */
#define TILE_WIDTH 8
#define TILE_MAP_TILE_WIDTH 32
#define TILE_MAP_WIDTH 256

uint8_t reverse_byte(uint8_t b);

void init_display_colors(display_colors *colors);

void dmg_render_sprite_pixels(gameboy *gb, gb_sprite *sprite);
void dmg_render_scanline(gameboy *gb);
void dmg_push_scanline_data(gameboy *gb);

void cgb_render_scanline(gameboy *gb);
void cgb_push_scanline_data(gameboy *gb);

#endif /* !CBOY_PPU_INTERNAL_H */
