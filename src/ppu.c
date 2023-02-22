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

/* gray shade color codes in ARGB8888 format,
 * but all tinted green to resemble physical
 * Game Boy colors
 */
 #define WHITE_G      0xff9bbc0f
 #define LIGHT_GRAY_G 0xff8bac0f
 #define DARK_GRAY_G  0xff306230
 #define BLACK_G      0xff0f380f


/* clock duration for a single frame of the Game Boy */
#define FRAME_CLOCK_DURATION 70224

/* Sprite rendering data */
typedef struct gb_sprite {
        uint8_t ypos, // sprite vertical pos + 16
                xpos, // sprite horizontal pos + 8
                tile_idx,
                ysize;
        
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
    ppu->curr_scanline_rendered = false;
    ppu->curr_frame_displayed = false;

    ppu->colors.white = WHITE;
    ppu->colors.light_gray = LIGHT_GRAY;
    ppu->colors.dark_gray = DARK_GRAY;
    ppu->colors.black = BLACK;
    ppu->colors.transparent = TRANSPARENT;
    ppu->colors.grayscale_mode = true;

    memset(ppu->frame_buffer, 0, FRAME_WIDTH * FRAME_HEIGHT * sizeof(uint32_t));

    return ppu;
}

void free_ppu(gb_ppu *ppu)
{
    free(ppu);
}

// switch between grayscale and shades-of-green
// colors for a more authentic Game Boy feel
void toggle_display_colors(gb_ppu *ppu)
{
    // swapping from grayscale to "greenscale"
    if (ppu->colors.grayscale_mode)
    {
        ppu->colors.white = WHITE_G;
        ppu->colors.light_gray = LIGHT_GRAY_G;
        ppu->colors.dark_gray = DARK_GRAY_G;
        ppu->colors.black = BLACK_G;
    }
    else
    {
        ppu->colors.white = WHITE;
        ppu->colors.light_gray = LIGHT_GRAY;
        ppu->colors.dark_gray = DARK_GRAY;
        ppu->colors.black = BLACK;
    }

    // we toggled colors
    ppu->colors.grayscale_mode = !ppu->colors.grayscale_mode;
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
static inline uint32_t color_from_palette(display_colors *colors, uint8_t palette_reg,
                                          bool is_sprite, uint8_t color_idx)
{
    uint32_t color;
    switch((palette_reg >> (2 * color_idx)) & 0x3)
    {
        case 0x0:
            color = is_sprite ? colors->transparent : colors->white;
            break;
        case 0x1:
            color = colors->light_gray;
            break;
        case 0x2:
            color = colors->dark_gray;
            break;
        case 0x3:
            color = colors->black;
            break;
    }

    return color;
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

        buff[i] = color_from_palette(&gb->ppu->colors, bgp, false, color_index);
    }
}

// load pixel color data for the sprite line (8 bytes) to be rendered in the given buffer
// using the occupancy array to handle drawing priority when sprites overlap
static void render_sprite_pixels(gameboy *gb, gb_sprite *sprite, uint32_t *buff, bool *occupancy)
{
    display_colors *colors = &gb->ppu->colors;

    // The current scanline of the PPU's internal frame buffer.
    // This is needed to handle the "BG over OBJ" flag of each sprite
    uint32_t *ppu_scanline_buff = gb->ppu->frame_buffer + gb->ppu->ly * FRAME_WIDTH;

    // select which line of the sprite will be rendered
    uint8_t line_to_render = gb->ppu->ly + 16 - sprite->ypos;

    // each line of the tile is 2 bytes
    uint8_t lo = sprite->tile_data[2*line_to_render],
            hi = sprite->tile_data[2*line_to_render + 1];

    /* convert these bytes into the corresponding color
     * indices and then into actual colors
     *
     * See: https://gbdev.io/pandocs/Tile_Data.html
     */
    uint8_t color_index, mask, bitno, hi_bit, lo_bit, palette_reg;
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

        // the palette register lets us map color indices to colors
        palette_reg = read_byte(gb, sprite->palette_no ? OBP1_REGISTER : OBP0_REGISTER);

        uint32_t color = color_from_palette(colors, palette_reg, true, color_index);

        // use the sprite's xpos to determine where each pixel lies on the scanline
        // Recall: xpos is the sprite's horizontal position + 8
        uint8_t shifted_buff_idx = sprite->xpos + i;
        uint8_t buff_idx;
        if (shifted_buff_idx >= 8 && shifted_buff_idx < FRAME_WIDTH + 8)
        {
            buff_idx = shifted_buff_idx - 8; // no overflow because shifted_buff_idx >= 8
            // determine if the buffer pixel is already occupied by an opaque sprite pixel
            if (!occupancy[buff_idx])
            {
                // determine if the sprite's pixel is drawn over the BG and window
                // (bg_over_obj only applies for BG/window colors 1-3)
                if (!sprite->bg_over_obj || ppu_scanline_buff[buff_idx] == colors->white)
                {
                    buff[buff_idx] = color;
                    // a buffer pixel is occupied only by opaque pixels
                    occupancy[buff_idx] = color != colors->transparent;
                }
            }
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
static void render_loaded_sprites(gameboy *gb, gb_sprite sprites[], uint8_t n_sprites)
{
    uint32_t scanline_buff[FRAME_WIDTH] = {0};
    // to track if each pixel in the scanline buffer
    // is already populated by a sprite's opaque pixel
    bool occupancy[FRAME_WIDTH] = {false};

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
            curr_sprite->tile_data[offset] = read_byte(gb, base_tile_addr + offset);

        // perform xflip and yflip before rendering by adjusting xpos and ypos
        perform_sprite_reflections(curr_sprite);

        render_sprite_pixels(gb, curr_sprite, scanline_buff, occupancy);
    }

    // once all sprites are in our temp buffer, push into the PPU's frame buffer
    uint32_t *ppu_scanline_buff = gb->ppu->frame_buffer + gb->ppu->ly * FRAME_WIDTH;
    for (uint8_t i = 0; i < FRAME_WIDTH; ++i)
    {
        // only want to push visible opaque sprite pixels
        if (occupancy[i])
            ppu_scanline_buff[i] = scanline_buff[i];
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

        ypos = read_byte(gb, oam_base_addr + oam_offset); // sprite vertical pos + 16

        // current scanline is interior to the sprite
        if (shifted_ly >= ypos && shifted_ly < ypos + sprite_ysize)
        {
            xpos = read_byte(gb, oam_base_addr + oam_offset + 1);
            tile_idx = read_byte(gb, oam_base_addr + oam_offset + 2);
            flags = read_byte(gb, oam_base_addr + oam_offset + 3);

            sprites_to_render[sprite_count].ypos     = ypos; // sprite vertical pos + 16
            sprites_to_render[sprite_count].xpos     = xpos;
            sprites_to_render[sprite_count].tile_idx = tile_idx;
            sprites_to_render[sprite_count].ysize    = sprite_ysize;

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
            gb->ppu->frame_buffer[scanline_start + i] = gb->ppu->colors.white;
        }
    }

    // Render the window, if enabled. The background and
    // window enable bit overrides the window enable bit
    if (bg_and_window_enable_bit && window_enable_bit)
    {
        load_window_tiles(gb, bg_win_tile_data_area_bit, window_tile_map_area_bit);
    }

    // render the sprites, if enabled
    if (obj_enable_bit)
        load_sprites(gb, obj_size_bit);
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

    ++gb->ppu->frames_rendered;
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

// handle STAT interrupt requests based on PPU mode
static void handle_ppu_mode_stat_interrupts(gameboy *gb)
{
    uint8_t stat                 = gb->memory->mmap[STAT_REGISTER],
            ppu_mode             = stat & 0x03,
            oam_interrupt_bit    = (stat & 0x20) >> 5,
            vblank_interrupt_bit = (stat & 0x10) >> 4,
            hblank_interrupt_bit = (stat & 0x08) >> 3;

    bool request_stat_interrupt = false;
    switch (ppu_mode)
    {
        case 0x00:
            request_stat_interrupt = hblank_interrupt_bit;
            break;
        case 0x01:
            request_stat_interrupt = vblank_interrupt_bit;
            break;
        case 0x02:
            request_stat_interrupt = oam_interrupt_bit;
            break;
        case 0x03: // no interrupt for mode 3
            break;
    }

    if (request_stat_interrupt)
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

// Pauses the emulator as needed after each frame is displayed
// to maintain the appropriate Game Boy frame rate.
static inline void maintain_framerate(gameboy *gb)
{
    uint64_t curr_time = SDL_GetTicks64();
    if (curr_time < gb->next_frame_time)
        SDL_Delay(gb->next_frame_time - curr_time);

    gb->next_frame_time += GB_FRAME_DURATION_MS;
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
        display_frame(gb);
        gb->ppu->curr_frame_displayed = true;
        request_interrupt(gb, VBLANK);
        maintain_framerate(gb);
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
