#include "cboy/ppu.h"
#include "ppu_internal.h"

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
