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

static inline void usage(const char *progname)
{
    const char *usage_str = "Usage: %s [-m] [-b bootrom] <romfile>\n"
                            "Options:\n"
                            "-m    Force the emulator to run in monochrome mode\n"
                            "-b    Specify a boot ROM file to play before running the game ROM\n";
    LOG_ERROR(usage_str, progname);
}

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
    const char *progname = argv[0],
               *bootrom = NULL,
               *romfile;

    bool force_dmg = false;

    while ((opt = getopt(argc, argv, "mb:")) != -1)
    {
        switch (opt)
        {
            case 'b':
                bootrom = optarg;
                LOG_INFO("Boot ROM supplied: %s\n", bootrom);
                break;

            case 'm':
                force_dmg = true;
                break;

            case '?':
                if (optopt == 'b')
                    LOG_ERROR("Option '%c' specified but no boot ROM was given\n", optopt);
                else
                    LOG_ERROR("Unrecognized option: '%c'\n", optopt);
                // fallthrough

            default:
                usage(progname);
                return 1;
        }
    }

    // we don't allow extraneous non-option arguments
    if (optind != argc - 1)
    {
        usage(progname);
        return 1;
    }
    else // only one non-option argument -- the romfile
    {
        romfile = argv[optind];
    }

    gameboy *gb = init_gameboy(romfile, bootrom, force_dmg);

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

    print_button_mappings(gb->run_mode);

    report_volume_level(gb, true);

    run_gameboy(gb);

    save_cartridge_ram(gb->cart, romfile);

    LOG_INFO("\n\nFrames rendered: %" PRIu64 "\n", gb->ppu->frames_rendered);

    free_gameboy(gb);
    return 0;
}
