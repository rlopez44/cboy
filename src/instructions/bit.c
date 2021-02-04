// Implementation of the bit shift instructions

#include <stdint.h>
#include "cboy/instructions.h"
#include "cboy/gameboy.h"
#include "execute.h"

/* the Rotate Left Circular Accumulator instruction
 * ================================================
 * Rotate the A register to the left by one position, with
 * bit 7 moved to bit 0 and also stored into the carry flag.
 *
 * Flags affected
 * --------------
 * Zero Flag:          reset
 * Subtract Flag:      reset
 * Half Carry Flag:    reset
 * Carry Flag:         set according to result
 */
void rlca(gameboy *gb)
{
    uint8_t bit_seven = gb->cpu->reg->a >> 7;
    gb->cpu->reg->a = (gb->cpu->reg->a << 1) | bit_seven;
    set_flags(gb->cpu->reg, 0, 0, 0, bit_seven);
}

/* the Rotate Left Accumulator instruction
 * =======================================
 * Rotate the A register to the left by one position, with
 * the carry flag moved into bit 0 and bit 7 moved into
 * the carry flag.
 *
 * Flags affected
 * --------------
 * Zero Flag:          reset
 * Subtract Flag:      reset
 * Half Carry Flag:    reset
 * Carry Flag:         set according to result
 */
void rla(gameboy *gb)
{
    uint8_t bit_seven = gb->cpu->reg->a >> 7;
    gb->cpu->reg->a = (gb->cpu->reg->a << 1) | read_carry_flag(gb->cpu->reg);
    set_flags(gb->cpu->reg, 0, 0, 0, bit_seven);
}

/* the Rotate Right Circular Accumulator instruction
 * =================================================
 * Rotate the A register to the right by one position, with
 * bit 0 moved to bit 7 and also stored into the carry flag.
 *
 * Flags affected
 * --------------
 * Zero Flag:          reset
 * Subtract Flag:      reset
 * Half Carry Flag:    reset
 * Carry Flag:         set according to result
 */
void rrca(gameboy *gb)
{
    uint8_t bit_zero = gb->cpu->reg->a & 1;
    gb->cpu->reg->a = (bit_zero << 7) | (gb->cpu->reg->a >> 1);
    set_flags(gb->cpu->reg, 0, 0, 0, bit_zero);
}

/* the Rotate Right Accumulator instruction
 * ========================================
 * Rotate the A register to the right by one position, with
 * the carry flag moved into bit 7 and bit 0 moved into
 * the carry flag.
 *
 * Flags affected
 * --------------
 * Zero Flag:          reset
 * Subtract Flag:      reset
 * Half Carry Flag:    reset
 * Carry Flag:         set according to result
 */
void rra(gameboy *gb)
{
    uint8_t bit_zero = gb->cpu->reg->a  & 1;
    gb->cpu->reg->a = (read_carry_flag(gb->cpu->reg) << 7) | (gb->cpu->reg->a >> 1);
    set_flags(gb->cpu->reg, 0, 0, 0, bit_zero);
}
