#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <SDL.h>
#include "cboy/gameboy.h"
#include "cboy/ppu.h"
#include "cboy/memory.h"
#include "cboy/interrupts.h"
#include "cboy/log.h"

/* gray shade color codes in ARGB8888 format */
#define WHITE       0xffffffff
#define LIGHT_GRAY  0xffaaaaaa
#define DARK_GRAY   0xff555555
#define BLACK       0xff000000
#define TRANSPARENT 0x00000000

/* clock duration for a single frame of the Game Boy */
#define FRAME_CLOCK_DURATION 70224

gb_ppu *init_ppu(void)
{
    gb_ppu *ppu = malloc(sizeof(gb_ppu));

    if (ppu == NULL)
    {
        return NULL;
    }

    ppu->dot_clock = 0;
    ppu->scx = 0;
    ppu->scy = 0;
    ppu->ly = 0;
    ppu->wx = 0;
    ppu->wy = 0;
    ppu->curr_scanline_rendered = false;
    ppu->curr_frame_displayed = false;

    memset(ppu->frame_buffer, 0, FRAME_WIDTH * FRAME_HEIGHT * sizeof(uint32_t));

    return ppu;
}

void free_ppu(gb_ppu *ppu)
{
    free(ppu);
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

// load pixel color data for one line of the tile (8 pixels) into the given buffer
static void load_tile_pixels(gameboy *gb, uint16_t tile_addr, uint16_t lineno, uint32_t *buff)
{
    // each line of the tile is 2 bytes
    uint8_t lo = read_byte(gb, tile_addr + 2 * lineno),
            hi = read_byte(gb, tile_addr + 2 * lineno + 1);

    /* convert these bytes into the corresponding color
     * indices and then into actual colors
     *
     * See: https://gbdev.io/pandocs/#vram-tile-data
     * and https://gbdev.io/pandocs/#lcd-monochrome-palettes
     */
    uint8_t color_index, mask, bitno, hi_bit, lo_bit, bgp;
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

        // the BGP register lets us map color indices to colors
        bgp = read_byte(gb, BGP_REGISTER);

        switch ((bgp >> (2 * color_index)) & 0x3)
        {
            case 0x0:
                buff[i] = WHITE;
                break;
            case 0x1:
                buff[i] = LIGHT_GRAY;
                break;
            case 0x2:
                buff[i] = DARK_GRAY;
                break;
            case 0x3:
                buff[i] = BLACK;
                break;
        }
    }
}

// load appropriate background tiles into the frame buffer for a single scanline
static void load_bg_tiles(gameboy *gb, bool tile_data_area_bit, bool tile_map_area_bit)
{
    // get the appropriate 32x32 tile map's address in VRAM
    uint16_t base_map_addr = tile_map_area_bit ? 0x9c00 : 0x9800;

    /* Determine the offset from this base address using the SCX, SCY, and LY registers.
     * We need two offsets: the offset to the first tile that will be loaded, and
     * the pixel offset within that tile to give the first pixel that will be loaded.
     */
    uint16_t tile_addr,
             pixel_xoffset      = gb->ppu->scx % 256,                 // modulo to wrap around the map
             pixel_yoffset      = (gb->ppu->scy + gb->ppu->ly) % 256,
             tile_xoffset       = pixel_xoffset / 8,                  // tile is 8 pixels wide
             tile_yoffset       = pixel_yoffset / 8,
             tile_pixel_xoffset = pixel_xoffset % 8,                  // pixel xoffset within the tile
             tile_pixel_yoffset = pixel_yoffset % 8;


    // now that we have our offsets, we can start loading tiles and rendering pixels
    uint8_t tile_index, curr_pixels_to_load = 0, pixels_left_to_load = FRAME_WIDTH;
    uint32_t tile_pixel_buff[8] = {0}; // to hold pixel color data for one line of a tile
    uint16_t frame_buffer_offset;

    while (pixels_left_to_load > 0)
    {
        tile_index = read_byte(gb, base_map_addr + tile_yoffset * 32 + tile_xoffset);
        tile_addr = tile_addr_from_index(tile_data_area_bit, tile_index);

        // because we're rendering pixels for one scanline,
        // we only need to load one line of the 8x8 tile
        load_tile_pixels(gb, tile_addr, tile_pixel_yoffset, tile_pixel_buff);

        curr_pixels_to_load = 8 - tile_pixel_xoffset; // from offset to end of tile line

        // make sure we don't overflow the scanline
        // when we're near the end of it
        if (pixels_left_to_load < curr_pixels_to_load)
        {
            curr_pixels_to_load = pixels_left_to_load;
        }

        // load the appropriate pixels
        frame_buffer_offset = FRAME_WIDTH * (gb->ppu->ly + 1) - pixels_left_to_load;
        memcpy(gb->ppu->frame_buffer + frame_buffer_offset,
               tile_pixel_buff + tile_pixel_xoffset,
               curr_pixels_to_load * sizeof(uint32_t));

        // prepare to load values from the next tile
        pixels_left_to_load -= curr_pixels_to_load;
        tile_pixel_xoffset = (tile_pixel_xoffset + curr_pixels_to_load) % 8;
        tile_xoffset = (tile_xoffset + 1) % 32; // wrap arround the tile map
    }
}

// load appropriate window tiles into the frame buffer for a single scanline
static void load_window_tiles(gameboy *gb, bool tile_data_area_bit, bool tile_map_area_bit)
{
    // the window is only visible if WX is in 0..166 and WY is in 0..143
    bool window_is_visible = gb->ppu->wx <= 166 && gb->ppu->wy <= 143;

    // we only need to draw if the current scanline overlaps the window
    bool scanline_overlaps_window = gb->ppu->ly >= gb->ppu->wy;

    if (window_is_visible && scanline_overlaps_window)
    {
        uint16_t base_map_addr = tile_map_area_bit ? 0x9c00 : 0x9800;

        /* The window is not scrollable, it is always loaded from
         * the top left tile of its tilemap. We can only adjust
         * the position of the window on the screen.
         */
        uint16_t tile_addr,
                 pixel_yoffset      = gb->ppu->ly - gb->ppu->wy, // offset from top of window
                 tile_yoffset       = pixel_yoffset / 8,
                 tile_pixel_yoffset = pixel_yoffset % 8;

        uint32_t scanline_buff[FRAME_WIDTH] = {0};
        uint8_t tile_index;

        /* Load window pixels for the full scanline one tile at a time.
         * We read from 20 tiles to load one scanline (20 * 8 = FRAME_WIDTH).
         * These tiles are on a single row of the 32x32 tilemap.
         * Because we start at the top left of the tilemap, we don't need
         * to worry about wrapping around the map when loading the 20 tiles.
         */
        for (uint8_t tile_xoffset = 0; tile_xoffset < FRAME_WIDTH / 8; ++tile_xoffset)
        {
            tile_index = read_byte(gb, base_map_addr + tile_yoffset * 32 + tile_xoffset);
            tile_addr = tile_addr_from_index(tile_data_area_bit, tile_index);

            // load one line (8 pixels) from the tile into the scanline buffer
            load_tile_pixels(gb, tile_addr, tile_pixel_yoffset, scanline_buff + 8 * tile_xoffset);
        }

        /* Copy the visible portion of the completed scanline buffer to the
         * frame buffer. Because WX is the x coordinate + 7 of the top left
         * point of the window, WX < 7 means the window is shifted left,
         * rather than right, so we need to handle this case separately.
         */
        uint16_t frame_buffer_offset;
        uint8_t scanline_buffer_offset, visible_pixel_count;
        if (gb->ppu->wx < 7)
        {
            visible_pixel_count = FRAME_WIDTH - (7 - gb->ppu->wx);
            scanline_buffer_offset = 7 - gb->ppu->wx;
            frame_buffer_offset = gb->ppu->ly * FRAME_WIDTH;
        }
        else
        {
            visible_pixel_count = FRAME_WIDTH - (gb->ppu->wx - 7);
            scanline_buffer_offset = 0;
            frame_buffer_offset = gb->ppu->ly * FRAME_WIDTH + gb->ppu->wx - 7;
        }

        memcpy(gb->ppu->frame_buffer + frame_buffer_offset,
               scanline_buff + scanline_buffer_offset,
               visible_pixel_count * sizeof(uint32_t));
    }
}

// Render a single scanline into the frame buffer
void render_scanline(gameboy *gb)
{
    // get the bit info out of the LCDC register
    uint8_t lcdc = gb->memory->mmap[LCDC_REGISTER];
    // TODO: handle LCD disabling
    bool lcd_enable_bit            = lcdc & 0x80,
         window_tile_map_area_bit  = lcdc & 0x40,
         window_enable_bit         = lcdc & 0x20,
         bg_win_tile_data_area_bit = lcdc & 0x10,
         bg_tile_map_area_bit      = lcdc & 0x08,
         obj_size_bit              = lcdc & 0x04,
         obj_enable_bit            = lcdc & 0x02,
         bg_and_window_enable_bit  = lcdc & 0x01;

    // the starting index of the current scanline in the frame buffer
    uint16_t scanline_start = gb->ppu->ly * FRAME_WIDTH;

    // render the background, if enabled
    if (bg_and_window_enable_bit)
    {
        load_bg_tiles(gb, bg_win_tile_data_area_bit, bg_tile_map_area_bit);
    }
    else // background becomes blank (white)
    {
        for (int i = 0; i < FRAME_WIDTH; ++i)
        {
            gb->ppu->frame_buffer[scanline_start + i] = WHITE;
        }
    }

    // Render the window, if enabled. The background and
    // window enable bit overrides the window enable bit
    if (bg_and_window_enable_bit && window_enable_bit)
    {
        load_window_tiles(gb, bg_win_tile_data_area_bit, window_tile_map_area_bit);
    }

    // TODO: render the sprites, if enabled
}

// Display the current frame buffer to the screen
void display_frame(gameboy *gb)
{
    void *texture_pixels;
    int pitch; // length of one row in bytes

    if (SDL_LockTexture(gb->screen, NULL, &texture_pixels, &pitch) < 0)
    {
        LOG_ERROR("Error drawing to screen\n");
        exit(1);
    }

    // even though we have the pitch from the call to SDL_LockTexture,
    // I prefer to use FRAME_WIDTH and FRAME_HEIGHT for the memcpy
    memcpy(texture_pixels, gb->ppu->frame_buffer, FRAME_WIDTH * FRAME_HEIGHT * sizeof(uint32_t));
    SDL_UnlockTexture(gb->screen);
    SDL_RenderClear(gb->renderer);
    SDL_RenderCopy(gb->renderer, gb->screen, NULL, NULL);
    SDL_RenderPresent(gb->renderer);
}

/* Compare the LY and LYC registers. If the two values
 * are equal, then the LYC=LY flag in the STAT register
 * is set.
 *
 * If the LYC=LY interrupt enable bit in the STAT
 * register is set then a STAT interrupt is requested.
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

        if (stat & cmp_interrupt)
        {
            request_interrupt(gb, LCD_STAT);
        }
    }
    else
    {
        // reset the LYC=LY flag
        gb->memory->mmap[STAT_REGISTER] = stat & ~cmp_flag;
    }

    return equal_values;
}

// handle interrupt requests based on PPU mode
static void handle_ppu_mode_interrupts(gameboy *gb)
{
    uint8_t stat                 = gb->memory->mmap[STAT_REGISTER],
            ppu_mode             = stat & 0x03,
            oam_interrupt_bit    = (stat & 0x20) >> 5,
            vblank_interrupt_bit = (stat & 0x10) >> 4,
            hblank_interrupt_bit = (stat & 0x08) >> 3;

    switch (ppu_mode)
    {
        case 0x00:
            if (hblank_interrupt_bit)
            {
                request_interrupt(gb, LCD_STAT);
            }
            break;
        case 0x01:
            if (vblank_interrupt_bit)
            {
                request_interrupt(gb, VBLANK);
            }
            break;
        case 0x02:
            if (oam_interrupt_bit)
            {
                request_interrupt(gb, LCD_STAT);
            }
            break;
        case 0x03: // no interrupt for mode 3
            break;
    }
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

    gb->ppu->dot_clock += num_clocks;

    uint8_t ppu_mode = set_ppu_mode(gb);
    handle_ppu_mode_interrupts(gb);
    ly_compare(gb);

    // we render a scanline once we reach the HBLANK period
    if (ppu_mode == 0x00 && !gb->ppu->curr_scanline_rendered)
    {
        render_scanline(gb);
        gb->ppu->curr_scanline_rendered = true;
    }
    // we display the frame once we've reached the VBLANK period
    else if (ppu_mode == 0x01 && !gb->ppu->curr_frame_displayed)
    {
        display_frame(gb);
        gb->ppu->curr_frame_displayed = true;
    }

    // check if we're done with the current scanline
    if (gb->ppu->dot_clock - 456 * gb->ppu->ly > 456)
    {
        // make sure we wrap around from scanline 153 to scanline 0
        gb->ppu->ly = (gb->ppu->ly + 1) % 154;
        gb->ppu->curr_scanline_rendered = false;
    }

    // reset the dot clock after cycling through all 154
    // scanlines so we can track timings for the next frame
    if (gb->ppu->dot_clock > FRAME_CLOCK_DURATION)
    {
        gb->ppu->dot_clock %= FRAME_CLOCK_DURATION;
        gb->ppu->curr_frame_displayed = false;
    }
}
