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

/* The PUSH instruction
 * --------------------
 * Pushes the given 16-bit register onto the stack.
 */
void push(gameboy *gb, gb_instruction *inst)
{
    uint16_t to_push; // the value to push onto the stack
    switch (inst->op1)
    {
        case REG_BC:
            to_push = read_bc(gb->cpu->reg);
            break;

        case REG_DE:
            to_push = read_de(gb->cpu->reg);
            break;

        case REG_HL:
            to_push = read_hl(gb->cpu->reg);
            break;

        case REG_AF:
            to_push = read_af(gb->cpu->reg);
            break;
    }

    stack_push(gb, to_push);
}

/* The POP instruction
 * ===================
 * Pops the given 16-bit register from the stack.
 *
 * Flags affected (only POP AF)
 * ----------------------------
 * Zero Flag:          set from bit 7 of the popped low byte
 * Subtract Flag:      set from bit 6 of the popped low byte
 * Half Carry Flag:    set from bit 5 of the popped low byte
 * Carry Flag:         set from bit 4 of the popped low byte
 */
void pop(gameboy *gb, gb_instruction *inst)
{
    // the value that was popped
    uint16_t popped = stack_pop(gb);
    switch (inst->op1)
    {
        case REG_BC:
            write_bc(gb->cpu->reg, popped);
            break;

        case REG_DE:
            write_de(gb->cpu->reg, popped);
            break;

        case REG_HL:
            write_hl(gb->cpu->reg, popped);
            break;

        case REG_AF:
        {
            write_af(gb->cpu->reg, popped);

            // also need to set flags
            uint8_t lo = (uint8_t)popped;
            set_flags(gb->cpu->reg,
                      (lo >> 7) & 1,   // Zero
                      (lo >> 6) & 1,   // Subtract
                      (lo >> 5) & 1,   // Half Carry
                      (lo >> 4) & 1);  // Carry
            break;
        }
    }
}
