// Implementation of miscellaneous instructions

#include <stdint.h>
#include <stdbool.h>
#include "cboy/instructions.h"
#include "cboy/gameboy.h"
#include "execute.h"

/* The enable interrupts instruction
 * ---------------------------------
 * Enables interrupts by setting the IME flag.
 * The setting of this flag is delayed by one
 * instruction. It is only set after the
 * instruction following the EI.
 *
 * TODO: see about possibly inlining this function
 */
void ei(gameboy *gb)
{
    // set the delayed IME set indicator so we know
    // to set the IME after the next instruction
    gb->cpu->ime_delayed_set = true;
}

/* The disable interrupts instruction
 * ----------------------------------
 *  Disables interrupts by clearing the IME flag.
 *
 *  TODO: see about possibly inlining this function
 */
void di(gameboy *gb)
{
    gb->cpu->ime_flag = false;
}
