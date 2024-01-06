#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "cboy/gameboy.h"
#include "cboy/memory.h"
#include "cboy/ppu.h"
#include "ppu_internal.h"

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
static bool scanline_obj_occupancy[FRAME_WIDTH] = {false};

static uint16_t cgb_color_from_palette(gameboy *gb, int loc)
{
    uint8_t palette_reg = scanline_palette_info[loc];
    uint8_t color_idx = scanline_coloridx_info[loc];
    bool is_sprite = scanline_obj_occupancy[loc];

    uint8_t offset = 8*palette_reg + 2*color_idx;
    uint8_t lo, hi;
    if (is_sprite)
    {
        lo = gb->ppu->obj_pram[offset];
        hi = gb->ppu->obj_pram[offset + 1];
    }
    else
    {
        lo = gb->ppu->bg_pram[offset];
        hi = gb->ppu->bg_pram[offset + 1];
    }

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

// determine whether a given sprite pixel will be drawn
// See: https://gbdev.io/pandocs/Tile_Maps.html#bg-to-obj-priority-in-cgb-mode
static bool resolve_obj_priority(gb_ppu *ppu, gb_sprite *sprite, uint8_t pixel_loc)
{
    bool bg_win_prio = ppu->lcdc & 1;

    // if the pixel is already occupied by a sprite, it will not be overwritten
    if (scanline_obj_occupancy[pixel_loc])
        return false;

    if (!bg_win_prio)
        return true;

    if (!sprite->bg_over_obj && !scanline_bg_prio_info[pixel_loc])
        return true;

    return !scanline_coloridx_info[pixel_loc];
}

// load pixel color data for the sprite line (8 bytes) to be rendered,
// mixing the sprite's pixels with the background and window
void cgb_render_sprite_pixels(gameboy *gb, gb_sprite *sprite)
{
    // select which line of the sprite will be rendered
    uint8_t line_to_render = gb->ppu->ly + 16 - sprite->ypos;

    // each line of the tile is 2 bytes
    uint8_t lo = sprite->tile_data[2*line_to_render],
            hi = sprite->tile_data[2*line_to_render + 1];

    uint8_t color_index, mask, bitno, hi_bit, lo_bit;
    for (uint8_t i = 0; i < 8; ++i)
    {
        bitno = 7 - i;
        mask = 1 << bitno;
        hi_bit = (hi & mask) >> bitno;
        lo_bit = (lo & mask) >> bitno;
        color_index = (hi_bit << 1) | lo_bit;

        // Recall: xpos is the sprite's horizontal position + 8
        uint8_t shifted_pixel_loc = sprite->xpos + i;

        // pixel is offscreen
        if (shifted_pixel_loc < 8 || shifted_pixel_loc >= FRAME_WIDTH + 8)
            continue;

        // no overflow because shifted_pixel_loc >= 8
        uint8_t pixel_loc = shifted_pixel_loc - 8;

        if (resolve_obj_priority(gb->ppu, sprite, pixel_loc) && color_index)
        {
            scanline_coloridx_info[pixel_loc] = color_index;
            scanline_palette_info[pixel_loc] = sprite->palette_no;
            scanline_obj_occupancy[pixel_loc] = true;
        }
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

// load appropriate window tiles into the pixel data buffers for a single scanline
static void cgb_load_window_tiles(gameboy *gb)
{
    gb_ppu *ppu = gb->ppu;
    bool tile_data_area_bit = ppu->lcdc & 0x10;
    bool tile_map_area_bit  = ppu->lcdc & 0x40; // window tile map flag

    // the window is only visible if WX is in 0..166 and WY is in 0..143
    bool window_is_visible = ppu->wx <= 166 && ppu->wy <= 143;

    // we only need to draw if the current scanline overlaps the window
    bool scanline_overlaps_window = ppu->ly >= ppu->wy;

    if (!(window_is_visible && scanline_overlaps_window))
        return;

    uint16_t base_map_addr = tile_map_area_bit ? 0x9c00 : 0x9800;

    /* The window tile map is not scrollable -- it is always rendered
     * from the top left tile, offsetting by how many visible window
     * scanlines have been rendered so far this frame.
     */
    uint16_t tile_addr, tile_index_addr;
    uint16_t pixel_yoffset      = ppu->window_line_counter,
             tile_yoffset       = pixel_yoffset / TILE_WIDTH,
             tile_pixel_yoffset = pixel_yoffset % TILE_WIDTH;

    // we need one extra tile for when the window is shifted left
    uint8_t scanline_buff[FRAME_WIDTH + TILE_WIDTH] = {0};
    uint8_t scanline_pbuff[FRAME_WIDTH + TILE_WIDTH] = {0};
    uint8_t scanline_prio_buff[FRAME_WIDTH + TILE_WIDTH] = {0};
    uint8_t tile_index;

    for (uint8_t tile_xoffset = 0;
         tile_xoffset < 1 + (FRAME_WIDTH / TILE_WIDTH);
         ++tile_xoffset)
    {
        tile_index_addr = base_map_addr
                          + tile_yoffset * TILE_MAP_TILE_WIDTH
                          + tile_xoffset;
        tile_index = gb->memory->vram[0][tile_index_addr & VRAM_MASK];
        tile_addr = tile_addr_from_index(tile_data_area_bit, tile_index);

        // window map attributes for the corresponding tile index
        uint8_t attrs = gb->memory->vram[1][tile_index_addr & VRAM_MASK];
        parse_bg_attrs(attrs);

        uint8_t offset = TILE_WIDTH * tile_xoffset;
        load_tile_color_data(gb, tile_addr,
                             tile_pixel_yoffset,
                             scanline_buff + offset);

        for (int i = 0; i < 8; ++i)
        {
            scanline_pbuff[offset + i] = curr_bg_attrs.paletteno;
            scanline_prio_buff[offset + i] = curr_bg_attrs.priority;
        }
    }

    // Copy the visible portion of the window scanline to the frame buffer.
    // Shifts left cover the entire visible scanline (WX < 7) and shifts
    // right offset by that many pixels.
    uint16_t color_data_buffer_offset = 0;
    uint8_t visible_pixel_count = FRAME_WIDTH;
    uint8_t scanline_buffer_offset = ppu->wx >= 7 ? 0 : 7 - ppu->wx;
    if (ppu->wx > 7)
    {
        color_data_buffer_offset += ppu->wx - 7;
        visible_pixel_count -= ppu->wx - 7;
    }

    // color indices
    memcpy(scanline_coloridx_info + color_data_buffer_offset,
           scanline_buff + scanline_buffer_offset,
           visible_pixel_count);

    // palette numbers
    memcpy(scanline_palette_info + color_data_buffer_offset,
           scanline_pbuff + scanline_buffer_offset,
           visible_pixel_count);

    // priorities
    memcpy(scanline_bg_prio_info + color_data_buffer_offset,
           scanline_prio_buff + scanline_buffer_offset,
           visible_pixel_count);

    ++ppu->window_line_counter;
}

static void reset_object_occupancy(void)
{
    memset(scanline_obj_occupancy, 0,
           sizeof scanline_obj_occupancy);
}

void cgb_render_scanline(gameboy *gb)
{
    bool window_enable = gb->ppu->lcdc & 0x20;
    bool obj_enable    = gb->ppu->lcdc & 0x02;

    cgb_load_bg_tiles(gb);

    if (window_enable)
        cgb_load_window_tiles(gb);

    if (obj_enable)
        load_sprites(gb);
}

// translate the completed scanline data into
// colors and push into the PPU's frame buffer
void cgb_push_scanline_data(gameboy *gb)
{
    // the starting index of the current scanline in the frame buffer
    uint16_t scanline_start = gb->ppu->ly * FRAME_WIDTH;

    uint16_t color;
    for (uint16_t i = 0; i < FRAME_WIDTH; ++i)
    {
        color = cgb_color_from_palette(gb, i);

        gb->ppu->frame_buffer[scanline_start + i] = color;
    }

    reset_object_occupancy();
}
