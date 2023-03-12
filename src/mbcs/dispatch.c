#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "cboy/gameboy.h"
#include "cboy/mbc.h"
#include "dispatch.h"
#include "cboy/log.h"

#define EXIT_INCONSISTENT_STATE() \
    LOG_ERROR("Inconsistent state:" \
              " emulator running with an unsupported MBC.\n" \
              "This is a bug.\n"); \
    exit(1);

/* print out the cartridge MBC type */
void print_mbc_type(MBC_TYPE mbc_type)
{
    const char *mbc_type_c;

    switch (mbc_type)
    {
        case NO_MBC:
            mbc_type_c = "No MBC";
            break;

        case MBC1:
            mbc_type_c = "MBC1";
            break;

        case MBC2:
            mbc_type_c = "MBC2";
            break;

        case MBC3:
            mbc_type_c = "MBC3";
            break;

        case MBC5:
            mbc_type_c = "MBC5";
            break;

        case MBC6:
            mbc_type_c = "MBC6";
            break;

        case MBC7:
            mbc_type_c = "MBC7";
            break;

        case MMM01:
            mbc_type_c = "MMM01";
            break;

        case HuC1:
            mbc_type_c = "HuC1";
            break;

        case HuC3:
            mbc_type_c = "HuC3";
            break;

        case UNKNOWN_MBC:
        default:
            mbc_type_c = "Unknown MBC";
            break;
    }

    LOG_INFO("MBC Type: %s\n", mbc_type_c);
}

bool mbc_supported(MBC_TYPE mbc_type)
{
    bool supported;
    switch (mbc_type)
    {
        case NO_MBC:
        case MBC1:
        case MBC3:
            supported = true;
            break;

        default:
            supported = false;
            break;
    }

    return supported;
}

void init_mbc(MBC_TYPE mbc_type, cartridge_mbc *mbc)
{
    mbc->mbc_type = mbc_type;

    switch (mbc_type)
    {
        case MBC1:
            mbc->mbc1.ram_enabled = false;
            mbc->mbc1.rom_bankno = 0;
            mbc->mbc1.ram_bankno = 0;
            mbc->mbc1.bank_mode = false;
            break;

        case MBC3:
            mbc->mbc3.ram_and_timer_enabled = false;
            mbc->mbc3.rom_bankno = 0;
            mbc->mbc3.ram_or_rtc_select = 0;
            mbc->mbc3.rtc_latch = 0;
            break;

        // NO_MBC or unsupported MBC, will not
        // be used, no need to initialize
        default:
            break;
    }
}

uint8_t cartridge_read(gameboy *gb, uint16_t address)
{
    uint8_t value;
    switch(gb->cart->mbc_type)
    {
        case NO_MBC:
            value = no_mbc_read(gb, address);
            break;

        case MBC1:
            value = mbc1_read(gb, address);
            break;

        case MBC3:
            value = mbc3_read(gb, address);
            break;

        // unsupported MBCs - should not get here
        default:
            EXIT_INCONSISTENT_STATE();
            break;
    }

    return value;
}

void cartridge_write(gameboy *gb, uint16_t address, uint8_t value)
{
    switch(gb->cart->mbc_type)
    {
        case NO_MBC:
            no_mbc_write(gb, address, value);
            break;

        case MBC1:
            mbc1_write(gb, address, value);
            break;

        case MBC3:
            mbc3_write(gb, address, value);
            break;

        // unsupported MBCs - should not get here
        default:
            EXIT_INCONSISTENT_STATE();
            break;
    }
}
