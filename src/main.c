#define _POSIX_C_SOURCE 2 /* to get access to getopt inside unistd */

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
    const char *usage_str = "Usage: %s [-123456m] [-b bootrom] <romfile>\n"
                            "Options:\n"
                            "  -123456  Scale the window by 1x through 6x, respectively.\n"
                            "             By default, the window is scaled by %dx.\n"
                            "  -m       Force the emulator to run in monochrome mode.\n"
                            "  -b       Specify a boot ROM file to play before running the game ROM.\n";
    LOG_ERROR(usage_str, progname, DEFAULT_WINDOW_SCALE);
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
    const char *progname = argv[0];
    struct gb_init_args init_args = {
        .bootrom = NULL,
        .romfile = NULL,
        .force_dmg = false,
        .window_scale = DEFAULT_WINDOW_SCALE,
    };

    while ((opt = getopt(argc, argv, "123456mb:")) != -1)
    {
        switch (opt)
        {
            case 'b':
                init_args.bootrom = optarg;
                LOG_INFO("Boot ROM supplied: %s\n", init_args.bootrom);
                break;

            case 'm':
                init_args.force_dmg = true;
                break;

            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
                init_args.window_scale = opt - '1' + 1;
                break;

            case '?':
                if (optopt == 'b')
                    LOG_ERROR("Option '%c' specified but no boot ROM was given\n", optopt);
                else
                    LOG_ERROR("Unrecognized option: '%c'\n", optopt);
                // fallthrough

            default:
                usage(progname);
                return 2;
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
        init_args.romfile = argv[optind];
    }

    gameboy *gb = init_gameboy(&init_args);

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

    save_cartridge_ram(gb->cart, init_args.romfile);

    LOG_INFO("\n\nFrames rendered: %" PRIu64 "\n", gb->ppu->frames_rendered);

    free_gameboy(gb);
    return 0;
}
