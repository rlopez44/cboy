#include <stdint.h>
#include <string.h>
#include "cboy/common.h"
#include "cboy/cpu.h"
#include "cboy/log.h"
#include "cboy/gameboy.h"
#include "cboy/ppu.h"
#include "cboy/apu.h"

#ifdef DEBUG
// print out the current CPU register contents
void print_registers(gameboy *gb)
{
    // NOTE: the output is formatted such that it can be compared to the
    // emulation logs at https://github.com/wheremyfoodat/Gameboy-logs
    // when running the Blargg test ROMs.

    // format: [registers] (mem[PC] mem[PC+1] mem[PC+2] mem[PC+3])
    const char *fmt = "A: %02X F: %02X B: %02X C: %02X "
                      "D: %02X E: %02X H: %02X L: %02X "
                      "SP: %04X PC: 00:%04X (%02X %02X %02X %02X)\n";

    LOG_DEBUG(fmt,
             gb->cpu->reg->a,
             gb->cpu->reg->f,
             gb->cpu->reg->b,
             gb->cpu->reg->c,
             gb->cpu->reg->d,
             gb->cpu->reg->e,
             gb->cpu->reg->h,
             gb->cpu->reg->l,
             gb->cpu->reg->sp,
             gb->cpu->reg->pc,
             read_byte(gb, gb->cpu->reg->pc),
             read_byte(gb, gb->cpu->reg->pc + 1),
             read_byte(gb, gb->cpu->reg->pc + 2),
             read_byte(gb, gb->cpu->reg->pc + 3));
}
#endif /* DEBUG */
