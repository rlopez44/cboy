#include <stdbool.h>
#include "cboy/mbc.h"
#include "dispatch.h"

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
        else // RTC
        {
            // TODO: implement actual registers
            value = 0;
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
        // TODO: actually implement latching
        mbc->rtc_latch = value;
    else if (0xa000 <= address && address <= 0xbfff && mbc->ram_and_rtc_enabled) // RAM or RTC
    {
        if (mbc->ram_or_rtc_select <= 0x03) // RAM
        {
            uint8_t bankno = mbc->ram_or_rtc_select;
            if (bankno < gb->cart->num_ram_banks)
                gb->cart->ram_banks[bankno][address - 0xa000] = value;
        }
        else // RTC
        {
            // TODO
        }
    }
}
