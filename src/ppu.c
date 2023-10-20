#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <SDL.h>
#include "cboy/common.h"
#include "cboy/gameboy.h"
#include "cboy/ppu.h"
#include "cboy/interrupts.h"
#include "cboy/log.h"

/* tile map dimensions (32 tiles = 256 pixels) */
#define TILE_WIDTH 8
#define TILE_MAP_TILE_WIDTH 32
#define TILE_MAP_WIDTH 256

/* sentinel value to indicate the background and
 * window are disabled when rendering scanlines
 */
#define NO_PALETTE 0x0000

/* clock duration for a single frame of the Game Boy */
#define FRAME_CLOCK_DURATION 70224

#define NUM_DISPLAY_PALETTES 4

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

/* Use to sort sprites according to their drawing priority.
 *
 * Smaller X coordinate -> higher priority
 * Same X coordinate -> located first in OAM -> higher priority
 */
static int sprite_comp(const void *a, const void *b)
{
    const gb_sprite *sprite1 = a, *sprite2 = b;
    uint8_t xpos1 = sprite1->xpos,
            xpos2 = sprite2->xpos;
    uint8_t offset1 = sprite1->oam_offset,
            offset2 = sprite2->oam_offset;

    if (xpos1 != xpos2)
        return xpos1 < xpos2 ? -1 : 1;
    else if (offset1 != offset2)
        return offset1 < offset2 ? -1 : 1;

    return 0;
}

static void init_display_colors(display_colors *colors)
{
    colors->black = display_color_palettes[0];
    colors->dark_gray = display_color_palettes[1];
    colors->light_gray = display_color_palettes[2];
    colors->white = display_color_palettes[3];
    colors->palette_index = 0;
}

gb_ppu *init_ppu(void)
{
    gb_ppu *ppu = malloc(sizeof(gb_ppu));

    if (ppu == NULL)
    {
        return NULL;
    }

    ppu->frames_rendered = 0;
    ppu->dot_clock = 0;
    ppu->scx = 0;
    ppu->scy = 0;
    ppu->ly = 0;
    ppu->wx = 0;
    ppu->wy = 0;
    ppu->window_line_counter = 0;
    ppu->curr_scanline_rendered = false;
    ppu->curr_frame_displayed = false;
    ppu->lyc_stat_line = false;
    ppu->hblank_stat_line = false;
    ppu->vblank_stat_line = false;
    ppu->oam_stat_line = false;

    init_display_colors(&ppu->colors);

    memset(ppu->frame_buffer, 0, sizeof ppu->frame_buffer);
    memset(ppu->scanline_palette_buff,
           NO_PALETTE,
           sizeof ppu->scanline_palette_buff);
    memset(ppu->scanline_coloridx_buff,
           0,
           sizeof ppu->scanline_coloridx_buff);

    return ppu;
}

void free_ppu(gb_ppu *ppu)
{
    free(ppu);
}

/* Reset the PPU.
 *
 * Should be called when the LCD/PPU
 * enable bit in LCDC is reset.
 *
 * Resetting the PPU immediately resets LY
 * (with no LY=LYC check) and resets the PPU
 * clock, as well as reset to LCD mode 0.
 */
void reset_ppu(gameboy *gb)
{
    gb->ppu->ly = 0;
    gb->ppu->dot_clock = 0;
    gb->memory->mmap[STAT_REGISTER] &= 0xf8;
    gb->ppu->curr_scanline_rendered = false;
    gb->ppu->curr_frame_displayed = false;
    gb->ppu->lyc_stat_line = false;
    gb->ppu->hblank_stat_line = false;
    gb->ppu->vblank_stat_line = false;
    gb->ppu->oam_stat_line = false;
    gb->ppu->window_line_counter = 0;

    // resetting the PPU makes the screen go blank (white)
    for (uint16_t i = 0; i < FRAME_WIDTH*FRAME_HEIGHT; ++i)
        gb->ppu->frame_buffer[i] = gb->ppu->colors.white;
    display_frame(gb);
    LOG_DEBUG("PPU reset\n");
}

/* Returns the state of the PPU's "STAT interrupt line"
 *
 * See: https://gbdev.io/pandocs/Interrupt_Sources.html#int-48--stat-interrupt
 */
static inline bool stat_interrupt_line(gb_ppu *ppu)
{
    return ppu->lyc_stat_line
           | ppu->hblank_stat_line
           | ppu->vblank_stat_line
           | ppu->oam_stat_line;
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

/* Get the memory address of a tile given its index
 * and the tile data area bit from the LCDC register.
 * The state of this bit determines what base memory
 * address in VRAM to use for accessing tiles. This
 * bit also determines whether the tile index is
 * interpreted as an unsigned or signed tile offset
 * from the base memory address (see below).
 *
 * Tile data area bit state
 * ~~~~~~~~~~~~~~~~~~~~~~~~
 *   Set:   unsigned offset (0 to 255)
 *   Reset: signed offset (-128 to 127)
 *
 * We'll perform any signed arithmetic needed using
 * unsigned values. Therefore, if we're interpreting
 * the tile index as a signed offset, we must sign
 * extend it to 16 bits.
 *
 * See: https://gbdev.io/pandocs/#vram-tile-data
 */
static uint16_t tile_addr_from_index(bool tile_data_area_bit, uint8_t tile_index)
{
    uint16_t base_data_addr = tile_data_area_bit ? 0x8000 : 0x9000,
             tile_offset    = (uint16_t)tile_index;

    // sign extension needed
    if (!tile_data_area_bit && (tile_index & 0x80) >> 7)
    {
        tile_offset |= 0xff00;
    }

    return base_data_addr + 16 * tile_offset; // each tile is 16 bytes
}

// Returns a color given a palette register, color index,
// and whether this color will be for a sprite tile.
// Should be used on the completed scanline data as the final
// step before outputting pixels.
static inline uint16_t color_from_palette(gameboy *gb, uint16_t palette_reg, uint8_t color_idx)
{
    // account for the bg/window disabled sentinel value
    // to ensure all non-sprite pixels are set to white
    uint8_t palette = palette_reg == NO_PALETTE ? 0 : gb->memory->mmap[palette_reg];

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
    // each line of the tile is 2 bytes
    uint8_t lo = gb->memory->mmap[load_addr],
            hi = gb->memory->mmap[load_addr + 1];

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
static void render_sprite_pixels(gameboy *gb, gb_sprite *sprite)
{
    uint16_t *palette_buff = gb->ppu->scanline_palette_buff;
    uint8_t *coloridx_buff = gb->ppu->scanline_coloridx_buff;

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
        if (palette_buff[pixel_loc] == OBP0_REGISTER
            || palette_buff[pixel_loc] == OBP1_REGISTER)
            continue;

        // bg_over_obj only applies for BG/window colors 1-3
        bool sprite_is_drawn = !sprite->bg_over_obj || !coloridx_buff[pixel_loc];
        sprite_is_drawn = sprite_is_drawn && color_index; // color index 0 is transparent

        if (sprite_is_drawn)
        {
            coloridx_buff[pixel_loc] = color_index;
            palette_buff[pixel_loc] = sprite->palette_no
                                      ? OBP1_REGISTER
                                      : OBP0_REGISTER;
        }
    }
}

// load appropriate background tiles into the pixel data buffers for a single scanline
static void load_bg_tiles(gameboy *gb, bool tile_data_area_bit, bool tile_map_area_bit)
{
    // get the appropriate 32x32 tile map's address in VRAM
    uint16_t base_map_addr = tile_map_area_bit ? 0x9c00 : 0x9800;

    // determine Y offset inside tile map based on current LY and SCY
    uint16_t pixel_yoffset      = (gb->ppu->scy + gb->ppu->ly) % TILE_MAP_WIDTH,
             tile_xoffset       = gb->ppu->scx / TILE_WIDTH,
             tile_pixel_xoffset = gb->ppu->scx % TILE_WIDTH, // offset within the tile
             tile_yoffset       = pixel_yoffset / TILE_WIDTH,
             tile_pixel_yoffset = pixel_yoffset % TILE_WIDTH;

    // traverse tile map until we've loaded a full frame width of pixels
    uint8_t tile_index, pixels_to_load, pixels_remaining = FRAME_WIDTH;
    uint16_t tile_addr;
    uint8_t tile_color_data[TILE_WIDTH] = {0};
    uint8_t *tile_color_data_load_start;
    for (uint16_t tileno = tile_xoffset;
         pixels_remaining > 0;
         tileno = (tileno + 1) % TILE_MAP_TILE_WIDTH)
    {
        tile_index = gb->memory->mmap[base_map_addr + TILE_MAP_TILE_WIDTH * tile_yoffset + tileno];
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

        memcpy(gb->ppu->scanline_coloridx_buff + FRAME_WIDTH - pixels_remaining,
               tile_color_data_load_start,
               pixels_to_load * sizeof(uint8_t));

        pixels_remaining -= pixels_to_load;
    }

    for (uint8_t i = 0; i < FRAME_WIDTH; ++i)
        gb->ppu->scanline_palette_buff[i] = BGP_REGISTER;
}

// load appropriate window tiles into the pixel data buffers for a single scanline
static void load_window_tiles(gameboy *gb, bool tile_data_area_bit, bool tile_map_area_bit)
{
    // the window is only visible if WX is in 0..166 and WY is in 0..143
    bool window_is_visible = gb->ppu->wx <= 166 && gb->ppu->wy <= 143;

    // we only need to draw if the current scanline overlaps the window
    bool scanline_overlaps_window = gb->ppu->ly >= gb->ppu->wy;

    if (!(window_is_visible && scanline_overlaps_window))
        return;

    uint16_t base_map_addr = tile_map_area_bit ? 0x9c00 : 0x9800;

    /* The window tile map is not scrollable -- it is always rendered
        * from the top left tile, offsetting by how many visible window
        * scanlines have been rendered so far this frame.
        */
    uint16_t tile_addr,
                pixel_yoffset      = gb->ppu->window_line_counter,
                tile_yoffset       = pixel_yoffset / TILE_WIDTH,
                tile_pixel_yoffset = pixel_yoffset % TILE_WIDTH;

    // we need one extra tile for when the window is shifted left
    uint8_t scanline_buff[FRAME_WIDTH + TILE_WIDTH] = {0};
    uint8_t tile_index;

    for (uint8_t tile_xoffset = 0;
         tile_xoffset < 1 + (FRAME_WIDTH / TILE_WIDTH);
         ++tile_xoffset)
    {
        tile_index = gb->memory->mmap[base_map_addr
                                      + tile_yoffset * TILE_MAP_TILE_WIDTH
                                      + tile_xoffset];
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
    uint8_t scanline_buffer_offset = gb->ppu->wx >= 7 ? 0 : 7 - gb->ppu->wx;
    if (gb->ppu->wx > 7)
    {
        color_data_buffer_offset += gb->ppu->wx - 7;
        visible_pixel_count -= gb->ppu->wx - 7;
    }

    memcpy(gb->ppu->scanline_coloridx_buff + color_data_buffer_offset,
           scanline_buff + scanline_buffer_offset,
           visible_pixel_count * sizeof(uint8_t));

    ++gb->ppu->window_line_counter;
}

// reverse the bits of the given byte
// See https://stackoverflow.com/a/2603254
static const uint8_t byte_reverse_lookup[16] = {
    0x0, 0x8, 0x4, 0xc, 0x2, 0xa, 0x6, 0xe,
    0x1, 0x9, 0x5, 0xd, 0x3, 0xb, 0x7, 0xf,
};
static inline uint8_t reverse_byte(uint8_t b)
{
    return (byte_reverse_lookup[b & 0xf] << 4) | byte_reverse_lookup[b >> 4];
}

// Reflect the sprite in the x and y directions if needed
static void perform_sprite_reflections(gb_sprite *sprite)
{
    if (sprite->yflip)
    {
        uint8_t tmp, top_offset, bot_offset;
        // vertical mirror = reflect about x-axis -> reverse tile data about the middle of the sprite
        for (uint8_t lineno = 0; lineno < sprite->ysize / 2; ++lineno)
        {
            for (uint8_t line_idx = 0; line_idx <= 1; ++line_idx)
            {
                top_offset = 2*lineno + line_idx;
                bot_offset = 2*(sprite->ysize - lineno - 1) + line_idx;

                tmp = sprite->tile_data[top_offset];
                sprite->tile_data[top_offset] = sprite->tile_data[bot_offset];
                sprite->tile_data[bot_offset] = tmp;
            }
        }
    }

    if (sprite->xflip)
    {
        // horizontal mirror = reflect about y-axis -> reverse bytes of each line's data
        // Recall: each line of the sprite is two bytes
        for (uint8_t lineno = 0; lineno < sprite->ysize; ++lineno)
        {
            sprite->tile_data[2*lineno] = reverse_byte(sprite->tile_data[2*lineno]);
            sprite->tile_data[2*lineno + 1] = reverse_byte(sprite->tile_data[2*lineno + 1]);
        }
    }
}

// Render the selected sprites from OAM
static void render_loaded_sprites(gameboy *gb, gb_sprite *sprites, uint8_t n_sprites)
{
    // apply drawing priority then draw
    qsort(sprites, n_sprites, sizeof(gb_sprite), &sprite_comp);

    for (uint8_t sprite_idx = 0; sprite_idx < n_sprites; ++sprite_idx)
    {
        uint16_t base_tile_addr;
        gb_sprite *curr_sprite = &sprites[sprite_idx];

        // read in the sprite's tile (two tiles if using 8x16 sprites)
        // Recall: each tile is 16 bytes in size
        // If using 8x8 sprites, the latter half of the sprite array is unused
        if (curr_sprite->ysize == 16)
            curr_sprite->tile_idx &= 0xfe; // hardware-enforced 8x16 indexing

        base_tile_addr = tile_addr_from_index(true, curr_sprite->tile_idx);
        for (uint16_t offset = 0; offset < curr_sprite->ysize * 2; ++offset)
            curr_sprite->tile_data[offset] = gb->memory->mmap[base_tile_addr + offset];

        // perform xflip and yflip before rendering by adjusting xpos and ypos
        perform_sprite_reflections(curr_sprite);

        render_sprite_pixels(gb, curr_sprite);
    }
}

// Select bytes from OAM to render for the current scanline
static void load_sprites(gameboy *gb, bool obj_size_bit)
{
    uint16_t oam_base_addr = 0xfe00;
    uint8_t ypos, xpos, tile_idx, flags;
    uint8_t sprite_ysize = obj_size_bit ? 16 : 8;

    // select the ten sprites to render for the current scan line from OAM
    gb_sprite sprites_to_render[10] = {0};
    uint8_t sprite_count = 0;
    uint8_t shifted_ly = gb->ppu->ly + 16; // to match the +16 offset inside ypos

    // NOTE: each sprite's attribute data is 4 bytes
    for (uint8_t oam_offset = 0x00; oam_offset < 0x9f; oam_offset += 4)
    {
        if (sprite_count >= 10)
            break;

        ypos = gb->memory->mmap[oam_base_addr + oam_offset]; // sprite vertical pos + 16

        // current scanline is interior to the sprite
        if (shifted_ly >= ypos && shifted_ly < ypos + sprite_ysize)
        {
            xpos = gb->memory->mmap[oam_base_addr + oam_offset + 1];
            tile_idx = gb->memory->mmap[oam_base_addr + oam_offset + 2];
            flags = gb->memory->mmap[oam_base_addr + oam_offset + 3];

            sprites_to_render[sprite_count].ypos     = ypos; // sprite vertical pos + 16
            sprites_to_render[sprite_count].xpos     = xpos;
            sprites_to_render[sprite_count].tile_idx = tile_idx;
            sprites_to_render[sprite_count].ysize    = sprite_ysize;

            sprites_to_render[sprite_count].oam_offset = oam_offset;

            // unpack sprite attributes (bits 0-3 are for CGB only)
            sprites_to_render[sprite_count].bg_over_obj = (flags >> 7) & 1;
            sprites_to_render[sprite_count].yflip       = (flags >> 6) & 1;
            sprites_to_render[sprite_count].xflip       = (flags >> 5) & 1;
            sprites_to_render[sprite_count].palette_no  = (flags >> 4) & 1;

            ++sprite_count;
        }
    }

    render_loaded_sprites(gb, sprites_to_render, sprite_count);
}

// translate the completed scanline data into
// colors and push into the PPU's frame buffer
static void push_scanline_data(gameboy *gb)
{
    uint16_t *scanline_palette_buff = gb->ppu->scanline_palette_buff;
    uint8_t *scanline_coloridx_buff = gb->ppu->scanline_coloridx_buff;

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

// Render a single scanline into the frame buffer
void render_scanline(gameboy *gb)
{
    // get the bit info out of the LCDC register
    uint8_t lcdc = gb->memory->mmap[LCDC_REGISTER];
    bool window_tile_map_area_bit  = lcdc & 0x40,
         window_enable_bit         = lcdc & 0x20,
         bg_win_tile_data_area_bit = lcdc & 0x10,
         bg_tile_map_area_bit      = lcdc & 0x08,
         obj_size_bit              = lcdc & 0x04,
         obj_enable_bit            = lcdc & 0x02,
         bg_and_window_enable_bit  = lcdc & 0x01;

    // render the background and window, if enabled
    if (bg_and_window_enable_bit)
    {
        load_bg_tiles(gb, bg_win_tile_data_area_bit,
                      bg_tile_map_area_bit);

        if (window_enable_bit)
            load_window_tiles(gb, bg_win_tile_data_area_bit,
                              window_tile_map_area_bit);
    }
    else // background becomes blank (white)
    {
        // all palettes and color indices set to 0 -> all white pixels
        memset(gb->ppu->scanline_palette_buff,
               NO_PALETTE, sizeof gb->ppu->scanline_palette_buff);
        memset(gb->ppu->scanline_coloridx_buff,
               0, sizeof gb->ppu->scanline_coloridx_buff);
    }

    // render the sprites, if enabled
    if (obj_enable_bit)
        load_sprites(gb, obj_size_bit);

    // Push the completed scanline into the frame buffer
    push_scanline_data(gb);
}

// Display the current frame buffer to the screen
void display_frame(gameboy *gb)
{
    void *texture_pixels;
    int pitch; // length of one row in bytes

    if (SDL_LockTexture(gb->screen, NULL, &texture_pixels, &pitch) < 0)
    {
        LOG_ERROR("Error drawing to screen: %s\n", SDL_GetError());
        exit(1);
    }

    // even though we have the pitch from the call to SDL_LockTexture,
    // I prefer to use FRAME_WIDTH and FRAME_HEIGHT for the memcpy
    memcpy(texture_pixels, gb->ppu->frame_buffer, sizeof gb->ppu->frame_buffer);
    SDL_UnlockTexture(gb->screen);
    SDL_RenderClear(gb->renderer);
    SDL_RenderCopy(gb->renderer, gb->screen, NULL, NULL);
    SDL_RenderPresent(gb->renderer);

    ++gb->ppu->frames_rendered;
}

/* Compare the LY and LYC registers. If the two values
 * are equal, then the LYC=LY flag in the STAT register
 * is set.
 *
 * If the LYC=LY interrupt enable bit in the STAT
 * register is set then a STAT interrupt is requested.
 *
 * This interrupt request occurs once every low-to-high
 * transition of the LY=LYC flag.
 */
static bool ly_compare(gameboy *gb)
{
    bool equal_values = gb->ppu->ly == gb->memory->mmap[LYC_REGISTER];

    uint8_t stat          = gb->memory->mmap[STAT_REGISTER],
            cmp_flag      = 0x04, // mask for the LYC=LY flag
            cmp_interrupt = 0x40; // mask for the LYC=LY interrupt enable bit

    if (equal_values)
    {
        // set the LYC=LY flag
        gb->memory->mmap[STAT_REGISTER] = stat | cmp_flag;

        if (stat & cmp_interrupt && !stat_interrupt_line(gb->ppu))
        {
            request_interrupt(gb, LCD_STAT);

            // we set this after requesting an interrupt, otherwise
            // the STAT interrupt line will always be high even
            // if modes 0-2 haven't requested an interrupt.
            gb->ppu->lyc_stat_line = true;
        }
    }
    else
    {
        // reset the LYC=LY flag
        gb->memory->mmap[STAT_REGISTER] = stat & ~cmp_flag;
        gb->ppu->lyc_stat_line = false;
    }

    return equal_values;
}

// handle STAT interrupt requests based on PPU mode
static void handle_ppu_mode_stat_interrupts(gameboy *gb)
{
    uint8_t stat                 = gb->memory->mmap[STAT_REGISTER],
            ppu_mode             = stat & 0x03,
            oam_interrupt_bit    = (stat & 0x20) >> 5,
            vblank_interrupt_bit = (stat & 0x10) >> 4,
            hblank_interrupt_bit = (stat & 0x08) >> 3;

    bool request_stat_interrupt = false;

    // snapshot this so we can update individual source lines
    // below without affecting our STAT interrupt request logic
    bool stat_interrupt_line_state = stat_interrupt_line(gb->ppu);

    switch (ppu_mode)
    {
        case 0x00:
            request_stat_interrupt = hblank_interrupt_bit;
            // mode 0 always follows mode 3 which
            // doesn't request STAT interrupts
            if (hblank_interrupt_bit)
                gb->ppu->hblank_stat_line = true;
            break;
        case 0x01:
            request_stat_interrupt = vblank_interrupt_bit;
            // mode 1 always follows mode 0
            gb->ppu->hblank_stat_line = false;
            if (vblank_interrupt_bit)
                gb->ppu->vblank_stat_line = true;
            break;
        case 0x02:
            request_stat_interrupt = oam_interrupt_bit;
            // mode 2 follows either mode 1 or mode 0
            gb->ppu->hblank_stat_line = false;
            gb->ppu->vblank_stat_line = false;
            if (oam_interrupt_bit)
                gb->ppu->oam_stat_line = true;
            break;
        case 0x03: // no interrupt for mode 3
            gb->ppu->hblank_stat_line = false;
            gb->ppu->vblank_stat_line = false;
            gb->ppu->oam_stat_line = false;
            break;
    }

    if (request_stat_interrupt && !stat_interrupt_line_state)
        request_interrupt(gb, LCD_STAT);
}

// set the appropriate mode in the STAT register
static uint8_t set_ppu_mode(gameboy *gb)
{
    uint8_t ppu_mode,
            old_stat        = gb->memory->mmap[STAT_REGISTER],
            mode_mask       = 0x03,
            masked_old_stat = old_stat & ~mode_mask;

    if (gb->ppu->ly > 143) // PPU is in VBLANK period
    {
        ppu_mode = 0x01;
    }
    else // PPU cycles through modes 2, 3, and 0 once every 456 clocks
    {
        uint16_t scanline_clock = gb->ppu->dot_clock % 456;
        if (scanline_clock <= 80) // Mode 2
        {
            ppu_mode = 0x02;
        }
        else if (scanline_clock <= 168) // Mode 3
        {
            ppu_mode = 0x03;
        }
        else // Mode 0
        {
            ppu_mode = 0x00;
        }
    }

    gb->memory->mmap[STAT_REGISTER] = masked_old_stat | ppu_mode;
    return ppu_mode;
}

// Run the PPU for the given number of clocks,
// handling all PPU-related logic as needed
void run_ppu(gameboy *gb, uint8_t num_clocks)
{
    // if the PPU isn't on then there's nothing to do
    uint8_t lcdc = gb->memory->mmap[LCDC_REGISTER];
    bool ppu_enabled = (lcdc >> 7) & 1;
    if (!ppu_enabled)
        return;

    for(; num_clocks; --num_clocks)
    {
        ++gb->ppu->dot_clock;

        uint8_t ppu_mode = set_ppu_mode(gb);
        handle_ppu_mode_stat_interrupts(gb);
        ly_compare(gb);

        // we render a scanline once we reach the HBLANK period
        if (ppu_mode == 0x00 && !gb->ppu->curr_scanline_rendered)
        {
            render_scanline(gb);
            gb->ppu->curr_scanline_rendered = true;
        }
        // we display the frame once we've reached the VBLANK period
        // we also need to request a vblank interrupt upon entering
        else if (ppu_mode == 0x01 && !gb->ppu->curr_frame_displayed)
        {
            request_interrupt(gb, VBLANK);
            display_frame(gb);
            gb->ppu->curr_frame_displayed = true;
            gb->ppu->window_line_counter = 0;
            gb->frame_presented_signal = true;
        }

        // check if we're done with the current scanline
        if (gb->ppu->dot_clock - 456 * gb->ppu->ly == 456)
        {
            // make sure we wrap around from scanline 153 to scanline 0
            gb->ppu->ly = (gb->ppu->ly + 1) % 154;
            gb->ppu->curr_scanline_rendered = false;
        }

        // reset the dot clock after cycling through all 154
        // scanlines so we can track timings for the next frame
        if (gb->ppu->dot_clock == FRAME_CLOCK_DURATION)
        {
            gb->ppu->dot_clock = 0;
            gb->ppu->curr_frame_displayed = false;
        }
    }
}
