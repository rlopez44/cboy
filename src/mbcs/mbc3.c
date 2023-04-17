#include <stdbool.h>
#include "cboy/gameboy.h"
#include "cboy/mbc.h"
#include "dispatch.h"

void fast_forward_rtc(cartridge_mbc3 *mbc, uint64_t num_seconds)
{
    for(; num_seconds; --num_seconds)
    {
        mbc->rtc_s = (mbc->rtc_s + 1) & 0x3f;
        if (mbc->rtc_s == 60)
        {
            mbc->rtc_s = 0;
            mbc->rtc_m = (mbc->rtc_m + 1) & 0x3f;

            if (mbc->rtc_m == 60)
            {
                mbc->rtc_m = 0;
                mbc->rtc_h = (mbc->rtc_h + 1) & 0x1f;

                if (mbc->rtc_h == 24)
                {
                    mbc->rtc_h = 0;
                    mbc->rtc_d = (mbc->rtc_d + 1) & 0x1ff;

                    if (!mbc->rtc_d)
                        mbc->day_carry = true;
                }
            }
        }
    }
}

void tick_rtc(gameboy *gb, uint8_t num_clocks)
{
    cartridge_mbc3 *mbc = &gb->cart->mbc->mbc3;

    if (mbc->rtc_halt)
        return;

    for(; num_clocks; --num_clocks)
    {
        --mbc->rtc_tick_timer;
        if (!mbc->rtc_tick_timer)
        {
            mbc->rtc_tick_timer = GB_CPU_FREQUENCY;

            /* It's possible for the seconds register
             * to have a value above 60 written to it.
             * The tick of the minutes register only
             * occurs at the exact value of 60.
             * Similar logic for the other registers.
             */
            mbc->rtc_s = (mbc->rtc_s + 1) & 0x3f;
            if (mbc->rtc_s == 60)
            {
                mbc->rtc_s = 0;
                mbc->rtc_m = (mbc->rtc_m + 1) & 0x3f;

                if (mbc->rtc_m == 60)
                {
                    mbc->rtc_m = 0;
                    mbc->rtc_h = (mbc->rtc_h + 1) & 0x1f;

                    if (mbc->rtc_h == 24)
                    {
                        mbc->rtc_h = 0;
                        mbc->rtc_d = (mbc->rtc_d + 1) & 0x1ff;

                        if (!mbc->rtc_d)
                            mbc->day_carry = true;
                    }
                }
            }
        }
    }
}

static void latch_rtc(cartridge_mbc3 *mbc)
{
    // order: S, M, H, DL, DH
    mbc->rtc_latched_values[0] = 0xc0 | (mbc->rtc_s & 0x3f);
    mbc->rtc_latched_values[1] = 0xc0 | (mbc->rtc_m & 0x3f);
    mbc->rtc_latched_values[2] = 0xe0 | (mbc->rtc_h & 0x1f);
    mbc->rtc_latched_values[3] = mbc->rtc_d & 0xff;
    mbc->rtc_latched_values[4] = ~0xc1
                                 | mbc->day_carry << 7
                                 | mbc->rtc_halt << 6
                                 | ((mbc->rtc_d >> 8) & 1);
}

static void handle_rtc_writes(cartridge_mbc3 *mbc, uint8_t value)
{
    switch (mbc->ram_or_rtc_select)
    {
        case 0x08:
            mbc->rtc_tick_timer = GB_CPU_FREQUENCY;
            mbc->rtc_s = value & 0x3f;
            mbc->rtc_latched_values[0] = 0xc0 | mbc->rtc_s;
            break;

        case 0x09:
            mbc->rtc_m = value & 0x3f;
            mbc->rtc_latched_values[1] = 0xc0 | mbc->rtc_m;
            break;

        case 0x0a:
            mbc->rtc_h = value & 0x1f;
            mbc->rtc_latched_values[2] = 0xe0 | mbc->rtc_h;
            break;

        case 0x0b:
            mbc->rtc_d &= 0x100;
            mbc->rtc_d |= value;
            mbc->rtc_latched_values[3] = value;
            break;

        case 0x0c:
            mbc->day_carry = (value >> 7) & 1;
            mbc->rtc_halt = (value >> 6) & 1;
            mbc->rtc_d &= 0xff;
            mbc->rtc_d |= (value & 1) << 8;
            mbc->rtc_latched_values[4] = ~0xc1 | (value & 0xc1);
            break;
    }
}

uint8_t mbc3_read(gameboy *gb, uint16_t address)
{
    cartridge_mbc3 *mbc = &gb->cart->mbc->mbc3;

    uint8_t value = 0xff; // open bus
    uint8_t bankno;
    // to ignore bits beyond those needed to address ROM banks
    uint16_t rom_bitmask = (1 << gb->cart->rom_banks_bitsize) - 1;

    if (address <= 0x3fff) // ROM bank 0
        value = gb->cart->rom_banks[0][address];
    else if (address <= 0x7fff) // ROM banks 0x01-0x7f
    {
        bankno = mbc->rom_bankno & rom_bitmask;
        value = gb->cart->rom_banks[bankno][address - 0x4000];
    }
    else if (0xa000 <= address && address <= 0xbfff && mbc->ram_and_rtc_enabled) // RAM or RTC
    {
        if (mbc->ram_or_rtc_select <= 0x03) // RAM
        {
            uint8_t bankno = mbc->ram_or_rtc_select;
            if (bankno < gb->cart->num_ram_banks)
                value = gb->cart->ram_banks[bankno][address - 0xa000];
        }
        else if (0x08 <= mbc->ram_or_rtc_select && mbc->ram_or_rtc_select <= 0x0c) // RTC
        {
            value = mbc->rtc_latched_values[mbc->ram_or_rtc_select - 0x08];
        }
    }

    return value;
}

void mbc3_write(gameboy *gb, uint16_t address, uint8_t value)
{
    cartridge_mbc3 *mbc = &gb->cart->mbc->mbc3;

    if (address <= 0x1fff) // RAM and Timer enable
    {
        if ((value & 0x0f) == 0x0a)
            mbc->ram_and_rtc_enabled = true;
        else if (!value)
            mbc->ram_and_rtc_enabled = false;
    }
    else if (address <= 0x3fff) // ROM bank number
    {
        uint8_t register_val = value & 0x7f;
        mbc->rom_bankno = register_val ? register_val : 0x01;
    }
    else if (address <= 0x5fff) // RAM bank or RTC select
    {
        // 0x00-0x03 = RAM bank select, 0x08-0x0c = RTC select
        bool valid_write = (value <= 0x03 && value < gb->cart->num_ram_banks)
                           || (0x08 <= value && value <= 0x0c);
        if (valid_write)
            mbc->ram_or_rtc_select = value;
    }
    else if (address <= 0x7fff) // RTC latch
    {
        // latching process: write of 0 followed by write of 1
        if (!mbc->rtc_latch && value == 0x01)
            latch_rtc(mbc);

        mbc->rtc_latch = value;
    }
    else if (0xa000 <= address && address <= 0xbfff && mbc->ram_and_rtc_enabled) // RAM or RTC
    {
        if (mbc->ram_or_rtc_select <= 0x03) // RAM
        {
            uint8_t bankno = mbc->ram_or_rtc_select;
            if (bankno < gb->cart->num_ram_banks)
                gb->cart->ram_banks[bankno][address - 0xa000] = value;
        }
        else if (0x08 <= mbc->ram_or_rtc_select && mbc->ram_or_rtc_select <= 0x0c) // RTC
        {
            handle_rtc_writes(mbc, value);
        }
    }
}
