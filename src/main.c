#include <stdio.h>
#include "cboy/cartridge.h"
#include "cboy/gameboy.h"
#include "cboy/log.h"

int main(int argc, const char *argv[])
{
#ifdef DEBUG
    // make sure our debug logs are printed
    ENABLE_DEBUG_LOGS();
#endif

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

    print_mbc_type(gb->cart);

    if (gb->cart->mbc_type != NO_MBC && gb->cart->mbc_type != MBC1)
    {
        LOG_INFO("NOTE: Only MBC1 is supported."
                 " This game will not run correctly\n");
    }

    run_gameboy(gb);

#ifdef DEBUG
    dump_memory(gb);
#endif

    free_gameboy(gb);
    return 0;
}
