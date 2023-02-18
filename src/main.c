#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include "cboy/cartridge.h"
#include "cboy/gameboy.h"
#include "cboy/joypad.h"
#include "cboy/log.h"

int main(int argc, char *argv[])
{
#ifdef DEBUG
    // make sure our debug logs are printed
    ENABLE_DEBUG_LOGS();
#endif

    LOG_INFO("CBoy -- A Game Boy Emulator\n"
             "---------------------------\n");

    // parse arguments
    opterr = false;
    int opt;
    const char *usage_str = "Usage: %s [-b bootrom] <romfile>\n",
               *progname = argv[0],
               *bootrom = NULL,
               *romfile;

    while ((opt = getopt(argc, argv, "b:")) != -1)
    {
        switch (opt)
        {
            case 'b':
                bootrom = optarg;
                LOG_INFO("Boot ROM supplied: %s\n", bootrom);
                break;

            case '?':
                if (optopt == 'b')
                    LOG_ERROR("Option '%c' specified but no boot ROM was given\n", optopt);
                else
                    LOG_INFO("Unrecognized option: '%c'\n", optopt);
                // fallthrough

            default:
                LOG_ERROR(usage_str, progname);
                return 1;
        }
    }
    
    // we don't allow extraneous non-option arguments
    if (optind != argc - 1)
    {
        LOG_ERROR(usage_str, progname);
        return 1;
    }
    else // only one non-option argument -- the romfile
    {
        romfile = argv[optind];
    }

    gameboy *gb = init_gameboy(romfile, bootrom);

    if (gb == NULL)
    {
        return 1;
    }

    print_rom_title(gb->cart);
    print_mbc_type(gb->cart);

    if (gb->cart->mbc_type != NO_MBC && gb->cart->mbc_type != MBC1)
    {
        LOG_INFO("NOTE: Only MBC1 is supported."
                 " This game will not run correctly\n");
    }

    print_button_mappings();

    run_gameboy(gb);

#ifdef DEBUG
    dump_memory(gb);
#endif

    free_gameboy(gb);
    return 0;
}
