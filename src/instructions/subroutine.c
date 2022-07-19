// Implementation of the jump and subroutine-related instruction sets

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "cboy/instructions.h"
#include "cboy/gameboy.h"
#include "cboy/cpu.h"
#include "cboy/memory.h"
#include "cboy/log.h"
#include "execute.h"

/* the jump instruction
 * --------------------
 * since some of the JP instructions are conditional and thus
 * have two possible durations, we need to return the
 * instruction duration for this instruction.
 */
uint8_t jp(gameboy *gb, gb_instruction *inst)
{
    uint8_t duration = 0;

    // check the second operand first. If it's NONE, we have one
    // of the two unconditional jumps out of the 6 instructions
    switch (inst->op2)
    {
        case NONE:
            duration = inst->duration;
            switch (inst->op1)
            {
                // jump to address specified by immediate value
                case IMM_16:
                {
                    // little-endian
                    uint8_t lo = read_byte(gb, (gb->cpu->reg->pc)++);
                    // no need to increment PC here since we're going to jump anyway
                    uint8_t hi = read_byte(gb, gb->cpu->reg->pc);

                    gb->cpu->reg->pc = ((uint16_t)hi << 8) | ((uint16_t)lo);
                    break;
                }

                // jump to address in HL register
                case REG_HL:
                    gb->cpu->reg->pc = read_hl(gb->cpu->reg);
                    break;

                default: // shouldn't get here
                    LOG_ERROR("Illegal argument in %s arg1 encountered. Exiting...\n", inst->inst_str);
                    exit(1);
            }
            break;

        case IMM_16:
        {
            uint8_t lo = read_byte(gb, (gb->cpu->reg->pc)++);
            // increment the PC after reading the hi byte of the
            // address because we might not be jumping, in which
            // case we need the PC to be pointing to the instruction
            // immediately after this one.
            uint8_t hi = read_byte(gb, (gb->cpu->reg->pc)++);

            uint16_t addr = ((uint16_t)hi << 8) | ((uint16_t)lo);

            bool will_jump = 0; // the condition for the jump
            switch (inst->op1)
            {
                case CC_C:
                    will_jump = read_carry_flag(gb->cpu->reg);
                    break;

                case CC_NC:
                    will_jump = !read_carry_flag(gb->cpu->reg);
                    break;

                case CC_Z:
                    will_jump = read_zero_flag(gb->cpu->reg);
                    break;

                case CC_NZ:
                    will_jump = !read_zero_flag(gb->cpu->reg);
                    break;

                default: // shouldn't get here
                    LOG_ERROR("Illegal argument in %s arg1, n16 encountered. Exiting...\n", inst->inst_str);
                    exit(1);
            }

            if (will_jump)
            {
                gb->cpu->reg->pc = addr;
                duration = inst->duration;
            }
            else
            {
                duration = inst->alt_duration;
            }
            break;
        }

        default: // shouldn't get here
            LOG_ERROR("Illegal argument in %s encountered. Exiting...\n", inst->inst_str);
            exit(1);
    }
    return duration;
}

/* the relative jump instruction
 * -----------------------------
 * following similar logic to the JP instruction,
 * we need to return the duration of this instruction
 */
uint8_t jr(gameboy *gb, gb_instruction *inst)
{
    uint8_t duration = 0;
    // the offset for the jump
    int8_t offset = (int8_t)read_byte(gb, (gb->cpu->reg->pc)++);

    // check the second operand first. If it's NONE, we have
    // the only unconditional jump out of the 5 instructions
    switch (inst->op2)
    {
        case NONE:
            duration = inst->duration;

            /* Jump by adding offset to the address of the
             * instruction following this one. Since we
             * incremented the PC after reading the byte
             * for the offset, the current value of the PC
             * is the address we add the offset to.
             *
             * NOTE: We explicity cast PC and offset to
             * avoid possible bugs due to implicit integer
             * conversions.
             */
            gb->cpu->reg->pc = (int32_t)gb->cpu->reg->pc + (int32_t)offset;
            break;

        case IMM_8:
        {
            // Same jump logic as the unconditional JR, but
            // here we also check if the jump condition is met
            bool will_jump = 0; // the condition for the jump
            switch (inst->op1)
            {
                case CC_C:
                    will_jump = read_carry_flag(gb->cpu->reg);
                    break;

                case CC_NC:
                    will_jump = !read_carry_flag(gb->cpu->reg);
                    break;

                case CC_Z:
                    will_jump = read_zero_flag(gb->cpu->reg);
                    break;

                case CC_NZ:
                    will_jump = !read_zero_flag(gb->cpu->reg);
                    break;

                default: // shouldn't get here
                    LOG_ERROR("Illegal argument in %s CC, n8 encountered. Exiting...\n", inst->inst_str);
                    exit(1);
            }

            if (will_jump)
            {
                gb->cpu->reg->pc = (int32_t)gb->cpu->reg->pc + (int32_t)offset;
                duration = inst->duration;
            }
            else
            {
                duration = inst->alt_duration;
            }
            break;
        }

        default: // shouldn't get here
            LOG_ERROR("Illegal argument in %s encountered. Exiting...\n", inst->inst_str);
            exit(1);
    }
    return duration;
}

/* the call instruction
 * --------------------
 * Some of the CALL instructions are conditional, so
 * we need to return the instruction duration explicitly
 */
uint8_t call(gameboy *gb, gb_instruction *inst)
{
    uint8_t duration = 0;

    // get the address to call
    uint8_t lo = read_byte(gb, (gb->cpu->reg->pc)++);
    uint8_t hi = read_byte(gb, (gb->cpu->reg->pc)++);
    uint16_t addr = ((uint16_t)hi << 8) | ((uint16_t)lo);

    // check the second operand first. If it's NONE, we have
    // the unconditional CALL, else a conditional CALL
    switch (inst->op2)
    {
        case NONE:
            duration = inst->duration;

            // push next instruction address onto the stack
            // so that a RET instruction can pop it later
            stack_push(gb, gb->cpu->reg->pc);

            // implicit jump instruction to the target address
            gb->cpu->reg->pc = addr;
            break;

        case IMM_16: // first operand is the condition
        {
            bool will_jump = 0; // the condition for the jump
            switch (inst->op1)
            {
                case CC_C:
                    will_jump = read_carry_flag(gb->cpu->reg);
                    break;

                case CC_NC:
                    will_jump = !read_carry_flag(gb->cpu->reg);
                    break;

                case CC_Z:
                    will_jump = read_zero_flag(gb->cpu->reg);
                    break;

                case CC_NZ:
                    will_jump = !read_zero_flag(gb->cpu->reg);
                    break;

                default: // shouldn't get here
                    LOG_ERROR("Illegal argument in %s CC, n16 encountered. Exiting...\n", inst->inst_str);
                    exit(1);
            }

            if (will_jump)
            {
                // push next instruction address onto the stack
                stack_push(gb, gb->cpu->reg->pc);

                // implicit jump to the target address
                gb->cpu->reg->pc = addr;

                duration = inst->duration;
            }
            else
            {
                duration = inst->alt_duration;
            }
            break;
        }

        default: // shouldn't get here
            LOG_ERROR("Illegal argument in %s encountered. Exiting...\n", inst->inst_str);
            exit(1);
    }
    return duration;
}

/* the restart (RST) instruction
 * -----------------------------
 * A faster version of the unconditional CALL instruction
 * for suitable target addresses. These target addresses
 * are the following:
 *
 *  0x00, 0x08, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38
 */
void rst(gameboy *gb, gb_instruction *inst)
{
    uint16_t addr = 0; // the target address
    switch (inst->op1)
    {
        case PTR_0x00:
            addr = 0x00;
            break;

        case PTR_0x08:
            addr = 0x08;
            break;

        case PTR_0x10:
            addr = 0x10;
            break;

        case PTR_0x18:
            addr = 0x18;
            break;

        case PTR_0x20:
            addr = 0x20;
            break;

        case PTR_0x28:
            addr = 0x28;
            break;

        case PTR_0x30:
            addr = 0x30;
            break;

        case PTR_0x38:
            addr = 0x38;
            break;

        default: // shouldn't get here
            LOG_ERROR("Illegal argument in %s encountered. Exiting...\n", inst->inst_str);
            exit(1);
    }

    // perform the call
    stack_push(gb, gb->cpu->reg->pc);
    gb->cpu->reg->pc = addr;
}

/* the return from subroutine (RET) instruction
 * --------------------------------------------
 * Some of the RET instructions are conditional, so
 * we need to return the instruction duration.
 *
 * This instruction pops the program counter from
 * the stack.
 */
uint8_t ret(gameboy *gb, gb_instruction *inst)
{
    uint8_t duration = 0;
    bool will_ret = 0; // whether the instruction will return
    switch (inst->op1)
    {
        case NONE: // the unconditional RET
            will_ret = 1;
            break;

        case CC_C:
            will_ret = read_carry_flag(gb->cpu->reg);
            break;

        case CC_NC:
            will_ret = !read_carry_flag(gb->cpu->reg);
            break;

        case CC_Z:
            will_ret = read_zero_flag(gb->cpu->reg);
            break;

        case CC_NZ:
            will_ret = !read_zero_flag(gb->cpu->reg);
            break;

        default: // shouldn't get here
            LOG_ERROR("Illegal argument in %s encountered. Exiting...\n", inst->inst_str);
            exit(1);
    }

    if (will_ret)
    {
        gb->cpu->reg->pc = stack_pop(gb);
        duration = inst->duration;
    }
    else
    {
        duration = inst->alt_duration;
    }

    return duration;
}

/* Return from subroutine and enable interrupts
 * --------------------------------------------
 * This can be thought of as executing an EI instruction
 * then an unconditional RET so that the IME flag is set
 * right after this instruction is executed.
 */
void reti(gameboy *gb)
{
    gb->cpu->reg->pc = stack_pop(gb);
    gb->cpu->ime_flag = true;
}
