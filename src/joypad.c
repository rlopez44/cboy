#include <SDL_events.h>
#include <SDL_keycode.h>
#include "cboy/common.h"
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

    // no buttons pressed and neither button set selected
    joypad->action_selected = false;
    joypad->dpad_selected = false;
    joypad->action_state = 0x0f;
    joypad->direction_state = 0x0f;

    return joypad;
}

// free the allocated memory for the joypad struct
void free_joypad(gb_joypad *joypad)
{
    free(joypad);
}

uint8_t report_button_states(gameboy *gb)
{
    gb_joypad *joypad = gb->joypad;

    uint8_t button_state;
    if (joypad->action_selected && joypad->dpad_selected)
    {
        button_state = joypad->action_state
                       & joypad->direction_state;
    }
    else if (joypad->action_selected)
    {
        button_state = joypad->action_state;
    }
    else if (joypad->dpad_selected)
    {
        button_state = joypad->direction_state;
    }
    else // neither button set selected
    {
        button_state = 0x0f;
    }

    // recall that 0 = selected
    return 0xc0
           | !joypad->action_selected << 5
           | !joypad->dpad_selected << 4
           | (button_state & 0x0f);
}

void update_button_set(gameboy *gb, uint8_t value)
{
    gb->joypad->dpad_selected = !(value & 0x10);
    gb->joypad->action_selected = !(value & 0x20);
}

// handle Game Boy key presses
void handle_keypress(gameboy *gb, SDL_KeyboardEvent *key)
{
    SDL_Keycode keycode = key->keysym.sym;

    // when a button is pressed, its key
    // state will switch from High to Low
    bool key_pressed = key->type == SDL_KEYDOWN;
    bool bit_val = !key_pressed;

    /**** special keys that aren't actually GB buttons ****/
    // cycle monochrome display colors (DMG mode only)
    if (keycode == SDLK_c && gb->run_mode == GB_DMG_MODE && key_pressed)
    {
        bool cycle_forward = !(key->keysym.mod & KMOD_SHIFT);
        cycle_display_colors(&gb->ppu->colors, cycle_forward);
        return;
    }
    else if (keycode == SDLK_EQUALS && key_pressed) // volume slider up
    {
        if (gb->volume_slider < 95)
            gb->volume_slider += 5;
        else
            gb->volume_slider = 100;

        report_volume_level(gb, false);
        return;
    }
    else if (keycode == SDLK_MINUS && key_pressed) // volume slider down
    {
        if (gb->volume_slider > 5)
            gb->volume_slider -= 5;
        else
            gb->volume_slider = 0;

        report_volume_level(gb, false);
        return;
    }
    else if (keycode == SDLK_TAB && key_pressed) // toggle FPS throttle
    {
        gb->throttle_fps = !gb->throttle_fps;
        return;
    }

    // determine bit mask and new bit value
    uint8_t mask, bit;
    switch(keycode)
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
    // NOTE: we have to update the button states before reading JOYP
    uint8_t joyp;
    switch (keycode)
    {
        case SDLK_s:
        case SDLK_w:
        case SDLK_a:
        case SDLK_d:
            gb->joypad->direction_state = (gb->joypad->direction_state & ~mask) | bit;
            joyp = report_button_states(gb);
            if (!(joyp & 0x10) && key_pressed)
                request_interrupt(gb, JOYPAD);
            break;

        case SDLK_RETURN:
        case SDLK_SPACE:
        case SDLK_j:
        case SDLK_k:
            gb->joypad->action_state = (gb->joypad->action_state & ~mask) | bit;
            joyp = report_button_states(gb);
            if (!(joyp & 0x20) && key_pressed)
                request_interrupt(gb, JOYPAD);
            break;
    }
}

void print_button_mappings(enum GAMEBOY_MODE gb_mode)
{
    const char *header = "Button Mappings\n"
                         "---------------";

    const char *cycle_msg = "Cycle display palettes: <c>/<Shift-c>";

    const char *base_msg = "Volume up/down: <Equals>/<Minus>\n"
                           "Toggle FPS throttle: <Tab>\n"
                           "B:      <j>\n"
                           "A:      <k>\n"
                           "Up:     <w>\n"
                           "Down:   <s>\n"
                           "Left:   <a>\n"
                           "Right:  <d>\n"
                           "Select: <Space>\n"
                           "Start:  <Enter>";

    if (gb_mode == GB_DMG_MODE)
        LOG_INFO("\n%s\n%s\n%s\n", header, cycle_msg, base_msg);
    else
        LOG_INFO("\n%s\n%s\n", header, base_msg);
}
