#include <SDL_events.h>
#include <SDL_keycode.h>
#include "cboy/gameboy.h"
#include "cboy/interrupts.h"
#include "cboy/joypad.h"
#include "cboy/memory.h"
#include "cboy/log.h"
#include "cboy/ppu.h"

/* Allocate memory for the joypad struct
 * and initialize button states.
 *
 * Returns NULL if the allocation fails
 */
gb_joypad *init_joypad(void)
{
    gb_joypad *joypad = malloc(sizeof(gb_joypad));

    if (joypad == NULL)
        return NULL;

    // no buttons pressed initially
    joypad->action_state = 0x0f;
    joypad->direction_state = 0x0f;

    return joypad;
}

// free the allocated memory for the joypad struct
void free_joypad(gb_joypad *joypad)
{
    free(joypad);
}

// report key states via the JOYP register
uint8_t report_button_states(gameboy *gb, uint8_t joyp)
{
    uint8_t button_state = 0x0f; // bottom four bits of JOYP

    // see which modes are selected (0 = selected)
    switch (joyp & 0x30)
    {
        case 0x30: // none selected
            button_state = 0x0f;
            break;
        case 0x20: // direction
            button_state = gb->joypad->direction_state;
            break;
        case 0x10: // action
            button_state = gb->joypad->action_state;
            break;
        case 0x00: // action & direction
            button_state = gb->joypad->action_state
                           & gb->joypad->direction_state;
            break;
    }

    // bits 6 and 7 are unused and always report set
    return 0xc0 | (joyp & 0x30) | (button_state & 0x0f);
}

// handle Game Boy key presses
void handle_keypress(gameboy *gb, SDL_KeyboardEvent *key)
{
    // when a button is pressed, its key
    // state will switch from High to Low
    bool key_pressed = key->type == SDL_KEYDOWN;
    bool bit_val = !key_pressed;

    // special key, not actually GB button
    if (key->keysym.sym == SDLK_c && key_pressed)
    {
        bool cycle_forward = !(key->keysym.mod & KMOD_SHIFT);
        cycle_display_colors(&gb->ppu->colors, cycle_forward);
        return;
    }

    // determine bit mask and new bit value
    uint8_t mask, bit;
    switch(key->keysym.sym)
    {
        case SDLK_s:
        case SDLK_RETURN:
            mask = 1 << 3;
            bit = bit_val << 3;
            break;
        case SDLK_w:
        case SDLK_SPACE:
            mask = 1 << 2;
            bit = bit_val << 2;
            break;
        case SDLK_a:
        case SDLK_j:
            mask = 1 << 1;
            bit = bit_val << 1;
            break;
        case SDLK_d:
        case SDLK_k:
            mask = 1 << 0;
            bit = bit_val << 0;
            break;

        default:
            return;
    }

    // handle button state change and Joypad interrupt (button selected and pressed)
    uint8_t joyp = gb->memory->mmap[JOYP_REGISTER];
    switch (key->keysym.sym)
    {
        case SDLK_s:
        case SDLK_w:
        case SDLK_a:
        case SDLK_d:
            gb->joypad->direction_state = (gb->joypad->direction_state & ~mask) | bit;
            if (!(joyp & 0x10) && key_pressed)
                request_interrupt(gb, JOYPAD);
            break;

        case SDLK_RETURN:
        case SDLK_SPACE:
        case SDLK_j:
        case SDLK_k:
            gb->joypad->action_state = (gb->joypad->action_state & ~mask) | bit;
            if (!(joyp & 0x20) && key_pressed)
                request_interrupt(gb, JOYPAD);
            break;
    }
}

void print_button_mappings(void)
{
    LOG_INFO("\n"
             "Button Mappings\n"
             "---------------\n"
             "Cycle display palettes: <c>/<Shift-c>\n"
             "B:      <j>\n"
             "A:      <k>\n"
             "Up:     <w>\n"
             "Down:   <s>\n"
             "Left:   <a>\n"
             "Right:  <d>\n"
             "Select: <Space>\n"
             "Start:  <Enter>\n");
}
