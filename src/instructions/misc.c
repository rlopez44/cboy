// Implementation of miscellaneous instructions

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "cboy/instructions.h"
#include "cboy/gameboy.h"
#include "cboy/cpu.h"
#include "cboy/log.h"
#include "cboy/interrupts.h"
#include "execute.h"

/* The enable interrupts instruction
 * ---------------------------------
 * Enables interrupts by setting the IME flag.
 * The setting of this flag is delayed by one
 * instruction. It is only set after the
 * instruction following the EI.
 */
void ei(gameboy *gb)
{
    // set the delayed IME set indicator so we know
    // to set the IME after the next instruction
    gb->cpu->ime_delayed_set = true;

    LOG_DEBUG("EI\n");
}

/* The disable interrupts instruction
 * ----------------------------------
 *  Disables interrupts by clearing the IME flag.
 */
void di(gameboy *gb)
{
    gb->cpu->ime_flag = false;

    LOG_DEBUG("DI\n");
}

/* The PUSH instruction
 * --------------------
 * Pushes the given 16-bit register onto the stack.
 */
void push(gameboy *gb, gb_instruction *inst)
{
    uint16_t to_push = 0; // the value to push onto the stack
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

        default: // shouldn't get here
            LOG_ERROR("Illegal argument in %s encountered. Exiting...\n", inst->inst_str);
            exit(1);
    }

    stack_push(gb, to_push);

    LOG_DEBUG("%s %s\n", inst->inst_str, operand_strs[inst->op1]);
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

        default: // shouldn't get here
            LOG_ERROR("Illegal argument in %s encountered. Exiting...\n", inst->inst_str);
            exit(1);
    }

    LOG_DEBUG("%s %s\n", inst->inst_str, operand_strs[inst->op1]);
}

/* The Decimal Adjust Accumulator instruction
 * ==========================================
 *
 * Background info on BCD (Binary-Coded Decimal)
 * ---------------------------------------------
 *  In BCD, each nibble of a value is used to
 *  represent a digit of a number in decimal (each
 *  nibble will be between 0x0 and 0x9 instead of
 *  0x0 and 0xf since the nibbles are supposed to
 *  represent decimal digits).
 *
 *  Examples of BCD representations:
 *   99 = 1001 (9) 1001 (9)
 *   32 = 0011 (3) 0010 (2)
 *   50 = 0101 (5) 0000 (0)
 *
 * Instruction Behavior
 * --------------------
 *  The GBZ80's arithmetic instructions perform binary
 *  addition/subtraction on their operands. Thus, when
 *  one of these instructions is used on BCD values,
 *  we must apply a correction to account for binary
 *  arithmetic being used on these BCD values. If this
 *  correction isn't applied, the result of the arithmetic
 *  instruction may not be a valid BCD value and if it is
 *  valid, it may not be the correct BCD value.
 *
 *  The purpose of this instruction is to perform this
 *  adjustment to the value in the accumulator. It should
 *  be used immediately after an arithmetic instruction
 *  on BCD values to avoid the flags changing due to other
 *  instructions.
 *
 *  For an explanation of the DAA instruction's
 *  behavior, see:
 *  https://ehaskins.com/2018-01-30%20Z80%20DAA/
 *
 *  The instruction's behavior can be summarized by the
 *  following rules:
 *   If the previous instruction was an addition: add 6 to
 *   each digit that is greater than 9 or that carried.
 *   If the upper nibble becomes > 9 after a correction to
 *   the lower nibble, then the upper nibble will then need
 *   to be corrected as well. This occurs when A is in
 *   the range 0x9a-0x9f before corrections are applied.
 *   Hence, corrections to the upper nibble always occur
 *   whenever A > 0x99 before corrections.
 *
 *   If the previous instruction was a subtraction: subtract
 *   6 from each digit that is > 9 or that borrowed. Unlike
 *   for addition, we don't need to check for digits > 9 since
 *   a subtraction between two valid BCD digits (0-9) can only
 *   give a result > 9 if a borrow occurred. Hence, the value
 *   of A doesn't matter for subtraction, as all the information
 *   we need for the adjustment is in the Carry and Half Carry
 *   flags. Because the smallest value that can occur from a
 *   borrow is 7 (0x0 - 0x9 =(borrow) 0x10 - 0x9 = 0x7),
 *   subtracting 6 from a digit that borrowed will not result in
 *   any new borrows.
 *
 * Flags Affected
 * --------------
 *  Zero Flag:          set if result is 0
 *  Half Carry Flag:    reset
 *  Carry Flag:         set or reset depending on the operation (see below)
 *
 *  Carry Flag Logic
 *  ++++++++++++++++
 *   Previous instruction was an addition
 *   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *    First, note that if A > 0x99, then the upper nibble of A
 *    will always carry after adjustments. This is because A
 *    satisfies one of two conditions in this case:
 *     (a) The upper nibble is already > 0x9 and the required
 *         adjustment to this nibble causes a carry
 *
 *     (b) The upper nibble is 0x9 and the lower nibble is > 0x9.
 *         The required adjustment to the lower nibble causes a
 *         carry into the upper nibble, which makes the upper nibble
 *         > 0x9. Then condition (a) applies and a carry from the
 *         upper nibble occurs.
 *
 *    Hence, the Carry Flag is set if A > 0x99 or the Carry Flag
 *    was set during the previous instruction (the BCD adjustment
 *    does not change the fact that a carry occurred in the previous
 *    instruction). Otherwise, the Carry Flag was reset during the
 *    previous instruction and will remain reset after this instruction.
 *
 *   Previous instruction was a subtraction
 *   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *   The behavior of DAA for subtractions does not lead to
 *   borrowing during the adjustments. Hence, the Carry Flag
 *   retains its value from the previous instruction.
 */
void daa(gameboy *gb)
{
    bool carry = read_carry_flag(gb->cpu->reg),
         subtract = read_subtract_flag(gb->cpu->reg),
         half_carry = read_half_carry_flag(gb->cpu->reg);

    if (!subtract) // previous instruction was addition
    {
        /* Notice from the DAA description above that the condition
         * for setting the carry flag and adjusting the upper nibble
         * of A are the same. Hence we only need to check a single
         * condition for both of these things. Also, because the
         * correction to the upper nibble does not influence the
         * correction to the lower nibble, we are free to perform it
         * first. This also lets us avoid using a temp variable to
         * store the original value of A, because if we adjust the
         * lower nibble first we would have to deal with wrap around
         * if A is in 0xfa-0xff.
         */
        if (carry || gb->cpu->reg->a > 0x99)
        {
            gb->cpu->reg->a += 0x60;
            set_carry_flag(gb->cpu->reg, 1);
        }

        /* Correct the lower nibble if needed. This may carry
         * into the upper nibble, but this is okay because we've
         * already checked if the upper nibble needed adjustment.
         */
        if (half_carry || (gb->cpu->reg->a & 0xf) > 0x9)
        {
            gb->cpu->reg->a += 0x6;
        }
    }
    else // previous instruction was subtraction
    {
        /* The Carry and Half Carry flags tell us what to correct.
         * Again, we can correct the upper nibble independently of
         * the lower nibble.
         */
        if (carry)
        {
            gb->cpu->reg->a -= 0x60;
        }

        if (half_carry)
        {
            gb->cpu->reg->a -= 0x6;
        }
    }

    // half carry and zero flag are always updated
    set_zero_flag(gb->cpu->reg, gb->cpu->reg->a == 0);
    set_half_carry_flag(gb->cpu->reg, 0);

    LOG_DEBUG("DAA\n");
}

/* The Set Carry Flag instruction
 * ==============================
 *
 * Flags affected
 * --------------
 *  Subtract Flag:      reset
 *  Half Carry Flag:    reset
 *  Carry Flag:         set
 */
void scf(gameboy *gb)
{
    // set the required flags
    set_subtract_flag(gb->cpu->reg, 0);
    set_half_carry_flag(gb->cpu->reg, 0);
    set_carry_flag(gb->cpu->reg, 1);

    LOG_DEBUG("SCF\n");
}

/* The Complement Carry Flag instruction
 * =====================================
 *
 * Flags affected
 * --------------
 *  Subtract Flag:      reset
 *  Half Carry Flag:    reset
 *  Carry Flag:         invert
 */
void ccf(gameboy *gb)
{
    set_carry_flag(gb->cpu->reg, read_carry_flag(gb->cpu->reg) ^ 1);

    // set the remaining flags
    set_subtract_flag(gb->cpu->reg, 0);
    set_half_carry_flag(gb->cpu->reg, 0);

    LOG_DEBUG("CCF\n");
}

/* The ComPLement accumulator instruction
 * ======================================
 *
 * Flags affected
 * --------------
 *  Subtract Flag:      set
 *  Half Carry Flag:    set
 */
void cpl(gameboy *gb)
{
    gb->cpu->reg->a ^= 0xff;

    // set flags
    set_subtract_flag(gb->cpu->reg, 1);
    set_half_carry_flag(gb->cpu->reg, 1);

    LOG_DEBUG("CPL\n");
}

/* The STOP instruction
 * ====================
 * Enters the CPU into a very low power standby mode.
 *
 * Here we only set a flag for the emulator that the
 * GB has been STOPped. The actual exiting out of this
 * state is handled via interrupts.
 *
 * Note that the STOP instruction is 2 bytes long,
 * but the second byte of the instruction is ignored.
 *
 * TODO: handle clock counter reset and disable when
 * entering STOP mode, as well as clock counter enable
 * when leaving STOP mode.
 */
void stop(gameboy *gb)
{
    // ignore the second byte of the instruction
    ++(gb->cpu->reg->pc);

    // set the Game Boy's stopped flag
    gb->is_stopped = true;

    LOG_DEBUG("STOP\n");
}

/* The HALT instruction
 * ====================
 * Enter the CPU into a low-power state.
 *
 * IF the IME flag is NOT set but an interrupt is
 * pending then the HALT immediately exits and
 * the HALT bug is triggered, where the CPU doesn't
 * increase the PC when it executes the next instruction.
 * This results in the byte after the HALT being read
 * twice.
 *
 * Here we set a flag for the emulator that the
 * CPU has been HALTed (if the HALT doesn't immediately
 * exit as described above). Exiting out of this state
 * is handled via interrupts.
 */
void halt(gameboy *gb)
{
    // IME not set and an interrupt is pending so
    // we never actually enter the HALTed state
    // and instead trigger the HALT bug
    if (!gb->cpu->ime_flag && pending_interrupts(gb))
    {
        gb->cpu->halt_bug = true;
        LOG_DEBUG("HALT bug\n");
    }
    else
    {
        gb->cpu->is_halted = true;
        LOG_DEBUG("HALT\n");
    }
}
