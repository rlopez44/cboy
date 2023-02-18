#include <SDL_events.h>
#include <SDL_keycode.h>
#include "cboy/gameboy.h"
#include "cboy/joypad.h"
#include "cboy/memory.h"
#include "cboy/log.h"

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

    joypad->up = false;
    joypad->left = false;
    joypad->down = false;
    joypad->right = false;
    joypad->a = false;
    joypad->b = false;
    joypad->select = false;
    joypad->start = false;

    return joypad;
}

// free the allocated memory for the joypad struct
void free_joypad(gb_joypad *joypad)
{
    free(joypad);
}

// report key states via the JOYP register
void report_button_states(gameboy *gb, BUTTON_REPORTING_MODE mode)
{
    // Recall: 0 = mode selected and 0 = button pressed
    // so we OR everything together and then negate
    // the resulting value for the correct bit pattern
    uint8_t joyp = mode;
    if (mode == DIRECTION)
    {
        joyp |= (gb->joypad->down << 3)
                | (gb->joypad->up << 2)
                | (gb->joypad->left << 1)
                | (gb->joypad->right);
    }
    else
    {
        joyp |= (gb->joypad->start << 3)
                | (gb->joypad->select << 2)
                | (gb->joypad->b << 1)
                | (gb->joypad->a);
    }

    gb->memory->mmap[JOYP_REGISTER] = ~joyp;
}

// handle Game Boy key presses
void handle_keypress(gb_joypad *joypad, SDL_KeyboardEvent *key)
{
    bool button_pressed = key->type == SDL_KEYDOWN;

    switch(key->keysym.sym)
    {
        case SDLK_w:
            joypad->up = button_pressed;
            break;
        case SDLK_a:
            joypad->left = button_pressed;
            break;
        case SDLK_s:
            joypad->down = button_pressed;
            break;
        case SDLK_d:
            joypad->right = button_pressed;
            break;
        case SDLK_j:
            joypad->b = button_pressed;
            break;
        case SDLK_k:
            joypad->a = button_pressed;
            break;
        case SDLK_SPACE:
            joypad->select = button_pressed;
            break;
        case SDLK_RETURN:
            joypad->start = button_pressed;
            break;

        default:
            break;
    }
}

void print_button_mappings(void)
{
    LOG_INFO("\n"
             "Button Mappings\n"
             "---------------\n"
             "B:      <j>\n"
             "A:      <k>\n"
             "Up:     <w>\n"
             "Down:   <s>\n"
             "Left:   <a>\n"
             "Right:  <d>\n"
             "Select: <Space>\n"
             "Start:  <Enter>\n");
}