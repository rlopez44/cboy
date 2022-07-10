#include <stdio.h>
#include "cboy/gameboy.h"
#include "cboy/log.h"

int main(int argc, const char *argv[])
{
    LOG_INFO("CBoy -- A Game Boy Emulator\n"
             "--------------------------\n");

    if (argc != 2)
    {
        LOG_ERROR("Usage: %s <rom_file_path>\n", argv[0]);
        return 1;
    }

    gameboy *gb = init_gameboy(argv[1]);

    if (gb == NULL)
    {
        return 1;
    }

    if (gb->cart->mbc != NO_MBC)
    {
        LOG_INFO("NOTE: MBCs are not yet supported."
                 " This game will not run correctly\n");
    }

    run_gameboy(gb);

#ifdef DEBUG
    dump_memory(gb);
#endif

    free_gameboy(gb);
    return 0;
}
