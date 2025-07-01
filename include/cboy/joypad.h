#ifndef GB_JOYPAD_H
#define GB_JOYPAD_H

#include <stdbool.h>
#include <SDL3/SDL_events.h>
#include "cboy/common.h"

/* Used to track the Joypad's state */
typedef struct gb_joypad {
    bool dpad_selected, action_selected;

    // D-pad: down, up, left, right
    uint8_t direction_state;

    // action buttons: start, select, b, a
    uint8_t action_state;
} gb_joypad;

typedef struct gameboy gameboy;

// handle Game Boy key presses
void handle_keypress(gameboy *gb, SDL_KeyboardEvent *key);

/* Report the value of the JOYP register
 *
 * JOYP bit meanings (0=selected)
 * ------------------------------
 * 7: unused (always set)
 * 6: unused (always set)
 * 5: select action buttons
 * 4: select D-pad
 * 3: start/down
 * 2: select/up
 * 1: B/left
 * 0: A/right
 */
uint8_t report_button_states(gameboy *gb);

// Update the selected button set given a value written to JOYP
void update_button_set(gameboy *gb, uint8_t value);

gb_joypad *init_joypad(void);

void free_joypad(gb_joypad *joypad);

void print_button_mappings(enum GAMEBOY_MODE gb_mode);
#endif /* GB_JOYPAD_H */
