#include <stdbool.h>
#include <stdint.h>
#include "cboy/gameboy.h"
#include "cboy/memory.h"
#include "cboy/ppu.h"
#include "ppu_internal.h"

#define VRAM_MASK 0x1fff

struct bg_attrs {
    bool priority;
    bool yflip;
    bool xflip;
    bool bankno;
    uint8_t paletteno;
};

static struct bg_attrs curr_bg_attrs;

// track palette and color index data for the scanline
// being rendered so we can mix the background, window,
// and sprites into a final image
static uint8_t scanline_palette_info[FRAME_WIDTH] = {0};
static uint8_t scanline_coloridx_info[FRAME_WIDTH] = {0};
static bool scanline_bg_prio_info[FRAME_WIDTH] = {false};

static uint16_t cgb_color_from_palette(gameboy *gb, uint8_t palette_reg, uint8_t color_idx)
{
    uint8_t offset = 8*palette_reg + 2*color_idx;
    uint8_t lo = gb->ppu->bg_pram[offset];
    uint8_t hi = gb->ppu->bg_pram[offset + 1];
    return (uint16_t)hi << 8 | lo;
}

// Extract BG map attributes for the current tile from the attribute byte
static void parse_bg_attrs(uint8_t attrs)
{
    curr_bg_attrs.priority  = (attrs >> 7) & 1;
    curr_bg_attrs.yflip     = (attrs >> 6) & 1;
    curr_bg_attrs.xflip     = (attrs >> 5) & 1;
    curr_bg_attrs.bankno    = (attrs >> 3) & 1;
    curr_bg_attrs.paletteno = attrs & 0x07;
}

// load pixel color data for one line of the tile (8 pixels) into the given buffer
static void load_tile_color_data(gameboy *gb, uint16_t tile_addr, uint8_t yoffset, uint8_t *buff)
{
    // each line of the tile is 2 bytes in VRAM
    uint8_t *vram_bank = gb->memory->vram[curr_bg_attrs.bankno];

    if (curr_bg_attrs.yflip)
        yoffset = 7 - yoffset;

    uint16_t load_addr = tile_addr + 2*yoffset; // two bytes per line
    uint8_t lo = vram_bank[load_addr & VRAM_MASK],
            hi = vram_bank[(load_addr + 1) & VRAM_MASK];

    if (curr_bg_attrs.xflip)
    {
        lo = reverse_byte(lo);
        hi = reverse_byte(hi);
    }

    // convert these bytes into the corresponding color indices
    // See: https://gbdev.io/pandocs/Tile_Data.html
    uint8_t color_index, mask, bitno, hi_bit, lo_bit;
    for (uint8_t i = 0; i < 8; ++i)
    {
        // Hi byte has the most significant bits of the color index
        // for each pixel, and lo byte has the least significant bits.
        // The leftmost bit represents the leftmost pixel in the line.
        bitno = 7 - i;
        mask = 1 << bitno;
        hi_bit = (hi & mask) >> bitno;
        lo_bit = (lo & mask) >> bitno;
        color_index = (hi_bit << 1) | lo_bit;
        buff[i] = color_index;
    }
}

// load appropriate background tiles into the pixel data buffers for a single scanline
static void cgb_load_bg_tiles(gameboy *gb)
{
    gb_ppu *ppu = gb->ppu;
    bool tile_data_area_bit = ppu->lcdc & 0x10;
    bool tile_map_area_bit  = ppu->lcdc & 0x08; // BG tile map flag

    // get the appropriate 32x32 tile map's address in VRAM
    uint16_t base_map_addr = tile_map_area_bit ? 0x9c00 : 0x9800;

    // determine Y offset inside tile map based on current LY and SCY
    uint16_t pixel_yoffset      = (ppu->scy + ppu->ly) % TILE_MAP_WIDTH,
             tile_xoffset       = ppu->scx / TILE_WIDTH,
             tile_pixel_xoffset = ppu->scx % TILE_WIDTH, // offset within the tile
             tile_yoffset       = pixel_yoffset / TILE_WIDTH,
             tile_pixel_yoffset = pixel_yoffset % TILE_WIDTH;

    // traverse tile map until we've loaded a full frame width of pixels
    uint8_t tile_index, pixels_to_load, pixels_remaining = FRAME_WIDTH;
    uint16_t tile_addr, tile_index_addr;
    uint8_t tile_color_data[TILE_WIDTH] = {0};
    uint8_t *tile_color_data_load_start;
    for (uint16_t tileno = tile_xoffset;
         pixels_remaining > 0;
         tileno = (tileno + 1) % TILE_MAP_TILE_WIDTH)
    {
        tile_index_addr = base_map_addr
                          + TILE_MAP_TILE_WIDTH * tile_yoffset
                          + tileno;
        tile_index = gb->memory->vram[0][tile_index_addr & VRAM_MASK];
        tile_addr = tile_addr_from_index(tile_data_area_bit, tile_index);

        // BG map attributes for the corresponding tile index
        uint8_t attrs = gb->memory->vram[1][tile_index_addr & VRAM_MASK];
        parse_bg_attrs(attrs);
        load_tile_color_data(gb, tile_addr, tile_pixel_yoffset, tile_color_data);

        // for the first tile loaded, throw away
        // enough leading pixels to account for SCX
        tile_color_data_load_start = tile_color_data;
        if (pixels_remaining == FRAME_WIDTH)
        {
            pixels_to_load = TILE_WIDTH - tile_pixel_xoffset;
            tile_color_data_load_start += tile_pixel_xoffset;
        }
        else if (pixels_remaining > TILE_WIDTH)
            pixels_to_load = TILE_WIDTH;
        else
            pixels_to_load = pixels_remaining;

        memcpy(scanline_coloridx_info + FRAME_WIDTH - pixels_remaining,
               tile_color_data_load_start,
               pixels_to_load);

        for (int i = 0; i < pixels_to_load; ++i)
        {
            int offset = FRAME_WIDTH - pixels_remaining + i;
            scanline_palette_info[offset] = curr_bg_attrs.paletteno;
            scanline_bg_prio_info[offset] = curr_bg_attrs.priority;
        }

        pixels_remaining -= pixels_to_load;
    }
}

void cgb_render_scanline(gameboy *gb)
{
    cgb_load_bg_tiles(gb);
}

// translate the completed scanline data into
// colors and push into the PPU's frame buffer
void cgb_push_scanline_data(gameboy *gb)
{
    // the starting index of the current scanline in the frame buffer
    uint16_t scanline_start = gb->ppu->ly * FRAME_WIDTH;

    uint8_t color_idx;
    uint8_t palette_reg;
    uint16_t color;
    for (uint16_t i = 0; i < FRAME_WIDTH; ++i)
    {
        palette_reg = scanline_palette_info[i];
        color_idx = scanline_coloridx_info[i];
        color = cgb_color_from_palette(gb, palette_reg, color_idx);

        gb->ppu->frame_buffer[scanline_start + i] = color;
    }

}
