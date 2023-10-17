#ifndef GB_JOYPAD_H
#define GB_JOYPAD_H

#include <stdbool.h>
#include <SDL_events.h>
#include "cboy/common.h"

/* use to track the Joypad's state (bottom nibble of JOYP) */
typedef struct gb_joypad {
    // D-pad: down, up, left, right
    uint8_t direction_state;

    // action buttons: start, select, b, a
    uint8_t action_state;
} gb_joypad;

typedef struct gameboy gameboy;

// handle Game Boy key presses
void handle_keypress(gameboy *gb, SDL_KeyboardEvent *key);

// report key states via JOYP register
uint8_t report_button_states(gameboy *gb, uint8_t joyp);

gb_joypad *init_joypad(void);

void free_joypad(gb_joypad *joypad);

void print_button_mappings(enum GAMEBOY_MODE gb_mode);
#endif /* GB_JOYPAD_H */
