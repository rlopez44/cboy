// Implementation of the jump and subroutine-related instruction sets

#include <stdint.h>
#include "cboy/instructions.h"
#include "cboy/cpu.h"
#include "cboy/memory.h"
#include "execute.h"

/* the jump instruction
 * --------------------
 * since some of the JP instructions are conditional and thus
 * have two possible durations, we need to return the
 * instruction duration for this instruction.
 */
uint8_t jp(gb_cpu *cpu, gb_instruction *inst)
{
    uint8_t duration;

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
                    uint8_t lo = read_byte(cpu->bus, (cpu->reg->pc)++);
                    // no need to increment PC here since we're going to jump anyway
                    uint8_t hi = read_byte(cpu->bus, cpu->reg->pc);

                    cpu->reg->pc = ((uint16_t)hi << 8) | ((uint16_t)lo);
                    break;
                }

                // jump to address in HL register
                case REG_HL:
                    cpu->reg->pc = read_hl(cpu->reg);
                    break;
            }
            break;

        case IMM_16:
        {
            uint8_t lo = read_byte(cpu->bus, (cpu->reg->pc)++);
            // increment the PC after reading the hi byte of the
            // address because we might not be jumping, in which
            // case we need the PC to be pointing to the instruction
            // immediately after this one.
            uint8_t hi = read_byte(cpu->bus, (cpu->reg->pc)++);
            
            uint16_t addr = ((uint16_t)hi << 8) | ((uint16_t)lo);

            uint8_t will_jump; // the condition for the jump
            switch (inst->op1)
            {
                case CC_C:
                    will_jump = read_carry_flag(cpu->reg);
                    break;

                case CC_NC:
                    will_jump = !read_carry_flag(cpu->reg);
                    break;

                case CC_Z:
                    will_jump = read_zero_flag(cpu->reg);
                    break;

                case CC_NZ:
                    will_jump = !read_zero_flag(cpu->reg);
                    break;
            }

            if (will_jump)
            {
                cpu->reg->pc = addr;
                duration = inst->duration;
            }
            else
            {
                duration=inst->alt_duration;
            }
            break;
        }
    }
    return duration;
}
