#include "cboy/log.h"

#ifdef DEBUG
#include "cboy/gameboy.h"

// dump the Game Boy's memory contents
void dump_memory(gameboy *gb)
{
    const char *dumpfile_path = "/tmp/gb.dump";
    FILE *dumpfile = fopen(dumpfile_path, "wb");

    if (dumpfile == NULL)
    {
        LOG_DEBUG("Error opening Game Boy memory dumpfile (%s)\n", dumpfile_path);
        return;
    }

    size_t bytes_written = fwrite(gb->memory->mmap, sizeof(uint8_t), MEMORY_MAP_SIZE, dumpfile);

    if (bytes_written != MEMORY_MAP_SIZE * sizeof(uint8_t))
    {
        LOG_DEBUG("Error dumping Game Boy memory into %s\n", dumpfile_path);
    }
    else
    {
        LOG_DEBUG("Game Boy memory dumped to %s\n", dumpfile_path);
    }

    fclose(dumpfile);
}

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
             gb->memory->mmap[gb->cpu->reg->pc],
             gb->memory->mmap[gb->cpu->reg->pc + 1],
             gb->memory->mmap[gb->cpu->reg->pc + 2],
             gb->memory->mmap[gb->cpu->reg->pc + 3]);
}
#endif /* DEBUG */
