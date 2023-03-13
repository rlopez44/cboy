#include <inttypes.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include "cboy/cartridge.h"
#include "cboy/gameboy.h"
#include "cboy/joypad.h"
#include "cboy/mbc.h"
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
                    LOG_ERROR("Unrecognized option: '%c'\n", optopt);
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
    print_mbc_type(gb->cart->mbc_type);

    if (!mbc_supported(gb->cart->mbc_type))
    {
        LOG_ERROR("Note: This MBC is not supported yet. Exiting...\n");
        exit(1);
    }

    print_button_mappings();

    run_gameboy(gb);

#ifdef DEBUG
    dump_memory(gb);
#endif

    save_cartridge_ram(gb->cart, romfile);

    // display the total number of frames rendered
    LOG_INFO("\nFrames rendered: %" PRIu64 "\n", gb->ppu->frames_rendered);

    free_gameboy(gb);
    return 0;
}
