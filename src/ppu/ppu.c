#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <SDL.h>
#include "cboy/common.h"
#include "cboy/gameboy.h"
#include "cboy/memory.h"
#include "cboy/ppu.h"
#include "cboy/interrupts.h"
#include "cboy/log.h"
#include "ppu_internal.h"

/* clock duration for a single frame of the Game Boy */
#define FRAME_CLOCK_DURATION 70224

/* Use to sort sprites according to their DMG drawing priority.
 *
 * Smaller X coordinate -> higher priority
 * Same X coordinate -> located first in OAM -> higher priority
 */
static int dmg_sprite_comp(const void *a, const void *b)
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

gb_ppu *init_ppu(enum GAMEBOY_MODE gb_mode)
{
    gb_ppu *ppu = calloc(1, sizeof(gb_ppu));

    if (ppu == NULL)
    {
        return NULL;
    }

    ppu->lcdc = 0x91;
    ppu->stat = 0x85;
    if (gb_mode == GB_DMG_MODE)
        ppu->dma = 0xff;
    ppu->bgp = 0xfc;
    ppu->obp0 = 0xff;
    ppu->obp1 = 0xff;

    // CGB-only I/O registers
    if (gb_mode == GB_CGB_MODE)
    {
        ppu->bcps = ppu->bcpd = 0xff;
        ppu->ocps = ppu->ocpd = 0xff;
        ppu->opri = 0xfe;
    }

    if (gb_mode == GB_DMG_MODE)
        init_display_colors(&ppu->colors);

    return ppu;
}

void free_ppu(gb_ppu *ppu)
{
    free(ppu);
}

uint8_t ppu_read(gameboy *gb, uint16_t address)
{
    gb_ppu *ppu = gb->ppu;
    uint8_t value = 0xff;
    switch (address)
    {
        case LCDC_REGISTER: value = ppu->lcdc; break;
        case STAT_REGISTER: value = ppu->stat; break;
        case SCY_REGISTER: value = ppu->scy; break;
        case SCX_REGISTER: value = ppu->scx; break;
        case LY_REGISTER: value = ppu->ly; break;
        case LYC_REGISTER: value = ppu->lyc; break;
        case DMA_REGISTER: value = ppu->dma; break;
        case BGP_REGISTER: value = ppu->bgp; break;
        case OBP0_REGISTER: value = ppu->obp0; break;
        case OBP1_REGISTER: value = ppu->obp1; break;
        case WY_REGISTER: value = ppu->wy; break;
        case WX_REGISTER: value = ppu->wx; break;
        case OPRI_REGISTER: value = ppu->opri; break;
        default: break;
    }

    return value;
}

void ppu_write(gameboy *gb, uint16_t address, uint8_t value)
{
    gb_ppu *ppu = gb->ppu;
    switch (address)
    {
        case LCDC_REGISTER:
            // reset the PPU when it's turned off (bit 7 of LCDC)
            if (!(value & 0x80))
                reset_ppu(gb);

            ppu->lcdc = value;
            break;

        case STAT_REGISTER:
        {
            // can only write to bits 3-6 of the STAT register
            uint8_t mask = 0x78;
            ppu->stat = (value & mask) | (ppu->stat & ~mask);
            break;
        }

        case SCY_REGISTER: ppu->scy = value; break;
        case SCX_REGISTER: ppu->scx = value; break;
        case LYC_REGISTER: ppu->lyc = value; break;

        case DMA_REGISTER:
            /* Begin the DMA transfer process by requesting it.
            * The written value must be between 0x00 and 0xdf,
            * otherwise no DMA transfer will occur.
            */
            if (value <= 0xdf && !gb->dma_requested)
            {
                LOG_DEBUG("DMA Requested\n");
                gb->dma_requested = true;
            }
            ppu->dma = value;
            break;

        case BGP_REGISTER: ppu->bgp = value; break;
        case OBP0_REGISTER: ppu->obp0 = value; break;
        case OBP1_REGISTER: ppu->obp1 = value; break;
        case WY_REGISTER: ppu->wy = value; break;
        case WX_REGISTER: ppu->wx = value; break;
        case OPRI_REGISTER: ppu->opri = 0xfe | (value & 1); break;
        default: break;
    }
}

/* Perform a DMA transfer from ROM or RAM to OAM
 * ---------------------------------------------
 * On hardware, the DMA transfer takes 160 m-cycles to complete,
 * but this function performs the transfer all at once. As such,
 * emulating the timing of the transfer should be handled by the
 * caller.
 *
 * The DMA source address is really the upper byte of the full
 * 16-bit starting address for the transfer (See below).
 *
 * Source and Destination
 * ~~~~~~~~~~~~~~~~~~~~~~
 * DMA source address: XX (<= 0xdf)
 * Source:          0xXX00 - 0xXX9f
 * Destination:     0xfe00 - 0xfe9f
 *
 */
void dma_transfer(gameboy *gb)
{
    if (gb->ppu->dma > 0xdf)
    {
        LOG_ERROR("Invalid DMA source hi: %02x."
                  " Must be between 00 and df\n",
                  gb->ppu->dma);
        exit(1);
    }

    uint16_t source, dest;
    uint8_t value;
    bool mbc_read;
    for (uint16_t lo = 0x0000; lo <= 0x009f; ++lo)
    {
        source = ((uint16_t)gb->ppu->dma << 8) | lo;
        dest = 0xfe00 | lo;
        mbc_read = source <= 0x7fff || (0xa000 <= source && source <= 0xbfff);

        // source may be in cartridge RAM/ROM
        if (mbc_read)
            value = cartridge_read(gb, source);
        else
            value = ram_read(gb, source);

        ram_write(gb, dest, value);
    }
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
    gb->ppu->stat &= 0xf8;
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
 * See: https://gbdev.io/pandocs/Tile_Data.html
 */
uint16_t tile_addr_from_index(bool tile_data_area_bit, uint8_t tile_index)
{
    uint16_t base_data_addr = tile_data_area_bit ? 0x8000 : 0x9000,
             tile_offset    = (uint16_t)tile_index;

    // interpret as signed offset, so sign extension needed
    if (!tile_data_area_bit && (tile_index & 0x80) >> 7)
    {
        tile_offset |= 0xff00;
    }

    return base_data_addr + 16 * tile_offset; // each tile is 16 bytes
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
    // Apply drawing priority then draw. Because objects are selected
    // out of OAM by scanning from start to end, they are already in
    // the correct ordering when using CGB priority
    if (gb->run_mode == GB_DMG_MODE || gb->ppu->opri & 1)
        qsort(sprites, n_sprites, sizeof(gb_sprite), dmg_sprite_comp);

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
            curr_sprite->tile_data[offset] = ram_read(gb, base_tile_addr + offset);

        // perform xflip and yflip before rendering by adjusting xpos and ypos
        perform_sprite_reflections(curr_sprite);

        // TODO: implement CGB sprite rendering
        dmg_render_sprite_pixels(gb, curr_sprite);
    }
}

// Select bytes from OAM to render for the current scanline
static void load_sprites(gameboy *gb)
{
    bool obj_size_bit = gb->ppu->lcdc & 0x04;
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

        ypos = ram_read(gb, oam_base_addr + oam_offset); // sprite vertical pos + 16

        // current scanline is interior to the sprite
        if (shifted_ly >= ypos && shifted_ly < ypos + sprite_ysize)
        {
            gb_sprite *curr_sprite = sprites_to_render + sprite_count;

            xpos = ram_read(gb, oam_base_addr + oam_offset + 1);
            tile_idx = ram_read(gb, oam_base_addr + oam_offset + 2);
            flags = ram_read(gb, oam_base_addr + oam_offset + 3);

            curr_sprite->ypos       = ypos; // sprite vertical pos + 16
            curr_sprite->xpos       = xpos;
            curr_sprite->tile_idx   = tile_idx;
            curr_sprite->ysize      = sprite_ysize;
            curr_sprite->oam_offset = oam_offset;

            // unpack sprite attributes
            curr_sprite->bg_over_obj = (flags >> 7) & 1;
            curr_sprite->yflip       = (flags >> 6) & 1;
            curr_sprite->xflip       = (flags >> 5) & 1;

            if (gb->run_mode == GB_DMG_MODE)
            {
                curr_sprite->palette_no = (flags >> 4) & 1;
            }
            else
            {
                curr_sprite->palette_no = flags & 0x7;
                curr_sprite->vram_bank = (flags >> 3) & 1;
            }

            ++sprite_count;
        }
    }

    render_loaded_sprites(gb, sprites_to_render, sprite_count);
}

// Render a single scanline into the frame buffer
static void render_scanline(gameboy *gb)
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
        memset(gb->ppu->scanline_palette_buff,
               DMG_NO_PALETTE, sizeof gb->ppu->scanline_palette_buff);
        memset(gb->ppu->scanline_coloridx_buff,
               0, sizeof gb->ppu->scanline_coloridx_buff);
    }

    if (obj_enable_bit)
        load_sprites(gb);

    dmg_push_scanline_data(gb);
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
    bool equal_values = gb->ppu->ly == gb->ppu->lyc;

    uint8_t stat          = gb->ppu->stat,
            cmp_flag      = 0x04, // mask for the LYC=LY flag
            cmp_interrupt = 0x40; // mask for the LYC=LY interrupt enable bit

    if (equal_values)
    {
        // set the LYC=LY flag
        gb->ppu->stat = stat | cmp_flag;

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
        gb->ppu->stat = stat & ~cmp_flag;
        gb->ppu->lyc_stat_line = false;
    }

    return equal_values;
}

// handle STAT interrupt requests based on PPU mode
static void handle_ppu_mode_stat_interrupts(gameboy *gb)
{
    uint8_t stat                 = gb->ppu->stat,
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
            old_stat        = gb->ppu->stat,
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

    gb->ppu->stat = masked_old_stat | ppu_mode;
    return ppu_mode;
}

// Run the PPU for the given number of clocks,
// handling all PPU-related logic as needed
void run_ppu(gameboy *gb, uint8_t num_clocks)
{
    // if the PPU isn't on then there's nothing to do
    bool ppu_enabled = (gb->ppu->lcdc >> 7) & 1;
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
