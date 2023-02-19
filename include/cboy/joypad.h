#ifndef GB_JOYPAD_H
#define GB_JOYPAD_H

#include <stdbool.h>
#include <SDL_events.h>

/* Button reporting modes of the joypad.
 *
 * Values are the bit masks for the
 * corresponding bit in the JOYP register.
 */
typedef enum BUTTON_REPORTING_MODE
{
    DIRECTION = 0x10, // Report Up, Down, Left, Right
    ACTION = 0x20,    // Report A, B, Select, Start
} BUTTON_REPORTING_MODE;

/* use to track the Joypad's state */
typedef struct gb_joypad {
    // D-pad
    bool up, left, down, right;

    // action buttons
    bool a, b, select, start;
} gb_joypad;

typedef struct gameboy gameboy;

// handle Game Boy key presses
void handle_keypress(gameboy *gb, SDL_KeyboardEvent *key);

// report key states via JOYP register
void report_button_states(gameboy *gb, BUTTON_REPORTING_MODE mode);

gb_joypad *init_joypad(void);

void free_joypad(gb_joypad *joypad);

void print_button_mappings(void);
#endif /* GB_JOYPAD_H */
