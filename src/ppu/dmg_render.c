#include "cboy/gameboy.h"
#include "cboy/log.h"
#include "cboy/ppu.h"
#include "ppu_internal.h"

#define NUM_DISPLAY_PALETTES 4

// track palette and color index data for the scanline
// being rendered so we can mix the background, window,
// and sprites into a final image
static uint16_t scanline_palette_buff[FRAME_WIDTH] = {0};
static uint8_t scanline_coloridx_buff[FRAME_WIDTH] = {0};

/* colors encoded in XBGR1555 format */
static const uint16_t display_color_palettes[4*NUM_DISPLAY_PALETTES] = {
    // grayscale
    // #000000, #555555, #aaaaaa, #ffffff,
    0x8000, 0xa94a, 0xd6b5, 0xffff,

    // green-tinted grayscale
    // #001000, #80a080, #c0d0c0, #f4fff4,
    0x8040, 0xc290, 0xe358, 0xfbfe,

    // pastel green shades
    // #081810, #396139, #84a563, #c9de8c,
    0x8861, 0x9d87, 0xb290, 0xc779,

    // acid green shades
    //#0f380f, #306230, #8bac0f, #9bbc0f,
    0x84e1, 0x9986, 0x86b1, 0x86f3,
};

void init_display_colors(display_colors *colors)
{
    colors->black = display_color_palettes[0];
    colors->dark_gray = display_color_palettes[1];
    colors->light_gray = display_color_palettes[2];
    colors->white = display_color_palettes[3];
    colors->palette_index = 0;
}

void cycle_display_colors(display_colors *colors, bool cycle_forward)
{
    uint8_t step = cycle_forward ? 1 : -1;
    uint8_t index = colors->palette_index;

    if (!index && !cycle_forward)
        index = NUM_DISPLAY_PALETTES;

    colors->palette_index = (index + step) % NUM_DISPLAY_PALETTES;

    colors->black = display_color_palettes[4*colors->palette_index];
    colors->dark_gray = display_color_palettes[4*colors->palette_index + 1];
    colors->light_gray = display_color_palettes[4*colors->palette_index + 2];
    colors->white = display_color_palettes[4*colors->palette_index + 3];
}

// Returns a color given a palette register, color index,
// and whether this color will be for a sprite tile.
// Should be used on the completed scanline data as the final
// step before outputting pixels.
static uint16_t color_from_palette(gameboy *gb, uint16_t palette_reg, uint8_t color_idx)
{
    // account for the bg/window disabled sentinel value
    // to ensure all non-sprite pixels are set to white
    uint8_t palette;
    switch(palette_reg)
    {
        case DMG_NO_PALETTE:
            palette = 0;
            break;

        case BGP_REGISTER:
            palette = gb->ppu->bgp;
            break;

        case OBP0_REGISTER:
            palette = gb->ppu->obp0;
            break;

        case OBP1_REGISTER:
            palette = gb->ppu->obp1;
            break;

        default:
            LOG_ERROR("Invalid palette register address: %04x\n", palette_reg);
            exit(1);
            break;
    }

    uint16_t color;
    switch((palette >> (2 * color_idx)) & 0x3)
    {
        case 0x0:
            color = gb->ppu->colors.white;
            break;
        case 0x1:
            color = gb->ppu->colors.light_gray;
            break;
        case 0x2:
            color = gb->ppu->colors.dark_gray;
            break;
        case 0x3:
            color = gb->ppu->colors.black;
            break;
    }

    return color;
}

// load pixel color data for one line of the tile (8 pixels) into the given buffer
static void load_tile_color_data(gameboy *gb, uint16_t load_addr, uint8_t *buff)
{
    // each line of the tile is 2 bytes in VRAM
    uint8_t lo = ram_read(gb, load_addr),
            hi = ram_read(gb, load_addr + 1);

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

// load pixel color data for the sprite line (8 bytes) to be rendered,
// mixing the sprite's pixels with the background and window
void dmg_render_sprite_pixels(gameboy *gb, gb_sprite *sprite)
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

        // if the pixel is already occupied by a sprite, it will not be overwritten
        if (scanline_palette_buff[pixel_loc] == OBP0_REGISTER
            || scanline_palette_buff[pixel_loc] == OBP1_REGISTER)
            continue;

        // bg_over_obj only applies for BG/window colors 1-3
        bool sprite_is_drawn = !sprite->bg_over_obj || !scanline_coloridx_buff[pixel_loc];
        sprite_is_drawn = sprite_is_drawn && color_index; // color index 0 is transparent

        if (sprite_is_drawn)
        {
            scanline_coloridx_buff[pixel_loc] = color_index;
            scanline_palette_buff[pixel_loc] = sprite->palette_no
                                               ? OBP1_REGISTER
                                               : OBP0_REGISTER;
        }
    }
}

// load appropriate background tiles into the pixel data buffers for a single scanline
void dmg_load_bg_tiles(gameboy *gb)
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
        tile_index = ram_read(gb, tile_index_addr);
        tile_addr = tile_addr_from_index(tile_data_area_bit, tile_index);
        load_tile_color_data(gb,
                             tile_addr + 2 * tile_pixel_yoffset, // two bytes per line
                             tile_color_data);

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

        memcpy(scanline_coloridx_buff + FRAME_WIDTH - pixels_remaining,
               tile_color_data_load_start,
               pixels_to_load * sizeof(uint8_t));

        pixels_remaining -= pixels_to_load;
    }

    for (uint8_t i = 0; i < FRAME_WIDTH; ++i)
        scanline_palette_buff[i] = BGP_REGISTER;
}

// load appropriate window tiles into the pixel data buffers for a single scanline
void dmg_load_window_tiles(gameboy *gb)
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
    uint8_t tile_index;

    for (uint8_t tile_xoffset = 0;
         tile_xoffset < 1 + (FRAME_WIDTH / TILE_WIDTH);
         ++tile_xoffset)
    {
        tile_index_addr = base_map_addr
                          + tile_yoffset * TILE_MAP_TILE_WIDTH
                          + tile_xoffset;
        tile_index = ram_read(gb, tile_index_addr);
        tile_addr = tile_addr_from_index(tile_data_area_bit, tile_index);

        load_tile_color_data(gb,
                             tile_addr + 2 * tile_pixel_yoffset, // two bytes per line
                             scanline_buff + TILE_WIDTH * tile_xoffset);
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

    memcpy(scanline_coloridx_buff + color_data_buffer_offset,
           scanline_buff + scanline_buffer_offset,
           visible_pixel_count * sizeof(uint8_t));

    ++ppu->window_line_counter;
}

void dmg_render_scanline(gameboy *gb)
{
    uint8_t lcdc = gb->ppu->lcdc;
    bool window_enable_bit         = lcdc & 0x20,
         obj_enable_bit            = lcdc & 0x02,
         bg_and_window_enable_bit  = lcdc & 0x01;

    if (bg_and_window_enable_bit)
    {
        dmg_load_bg_tiles(gb);

        if (window_enable_bit)
            dmg_load_window_tiles(gb);
    }
    else // background becomes blank (white)
    {
        // all palettes and color indices set to 0 -> all white pixels
        memset(scanline_palette_buff,
               DMG_NO_PALETTE, sizeof scanline_palette_buff);
        memset(scanline_coloridx_buff,
               0, sizeof scanline_coloridx_buff);
    }

    if (obj_enable_bit)
        load_sprites(gb);

}

// translate the completed scanline data into
// colors and push into the PPU's frame buffer
void dmg_push_scanline_data(gameboy *gb)
{
    // the starting index of the current scanline in the frame buffer
    uint16_t scanline_start = gb->ppu->ly * FRAME_WIDTH;

    uint8_t color_idx;
    uint16_t palette_reg;
    uint16_t color;
    for (uint16_t i = 0; i < FRAME_WIDTH; ++i)
    {
        palette_reg = scanline_palette_buff[i];
        color_idx = scanline_coloridx_buff[i];
        color = color_from_palette(gb, palette_reg, color_idx);

        gb->ppu->frame_buffer[scanline_start + i] = color;
    }

}
