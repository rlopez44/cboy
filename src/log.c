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
#endif /* DEBUG */
