#include "cboy/mbc.h"
#include "dispatch.h"

uint8_t mbc1_read(gameboy *gb, uint16_t address)
{
    cartridge_mbc1 *mbc = &gb->cart->mbc->mbc1;

    uint8_t value = 0xff; // open bus
    uint8_t bankno;
    // to ignore bits beyond those needed to address ROM banks
    uint16_t rom_bitmask = (1 << gb->cart->rom_banks_bitsize) - 1;

    if (address <= 0x3fff) // GB ROM bank 0
    {
        bankno = (mbc->bank_mode ? mbc->ram_bankno << 5 : 0) & rom_bitmask;
        value = gb->cart->rom_banks[bankno][address];
    }
    else if (address <= 0x7fff) // GB ROM bank 1
    {
        // a value of 0x00 for ROM_BANKNO behaves as if it were 0x01
        uint8_t adjusted_rom_bankno = mbc->rom_bankno ? mbc->rom_bankno : 0x01;

        bankno = ((mbc->ram_bankno << 5) | adjusted_rom_bankno) & rom_bitmask;

        value = gb->cart->rom_banks[bankno][address - 0x4000];
    }
    else if (0xa000 <= address && address <= 0xbfff && mbc->ram_enabled)// GB RAM bank
    {
        // 8KB RAM cartridges always access their single RAM bank
        bankno = mbc->bank_mode && gb->cart->num_ram_banks > 1 ? mbc->ram_bankno : 0;
        if (bankno < gb->cart->num_ram_banks)
            value = gb->cart->ram_banks[bankno][address - 0xa000];
    }

    return value;
}

void mbc1_write(gameboy *gb, uint16_t address, uint8_t value)
{
    cartridge_mbc1 *mbc = &gb->cart->mbc->mbc1;

    if (address <= 0x1fff) // RAM enable
        // any value with $A in the lower 4 bits enables RAM
        mbc->ram_enabled = (value & 0x0f) == 0x0a;
    else if (address <= 0x3fff) // ROM bank number
        mbc->rom_bankno = value & 0x1f;
    else if (address <= 0x5fff) // RAM bank number
    {
        mbc->ram_bankno = value & 0x03;
    }
    else if (address <= 0x7fff) // bank mode
        mbc->bank_mode = value;
    else if (0xa000 <= address && address <= 0xbfff && mbc->ram_enabled) // RAM
    {
        uint8_t bankno = mbc->bank_mode && gb->cart->num_ram_banks > 1 ? mbc->ram_bankno : 0;
        if (bankno < gb->cart->num_ram_banks)
            gb->cart->ram_banks[bankno][address - 0xa000] = value;
    }
}
