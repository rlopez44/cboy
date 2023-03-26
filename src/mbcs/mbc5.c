#include <stdint.h>
#include "cboy/mbc.h"
#include "dispatch.h"

uint8_t mbc5_read(gameboy *gb, uint16_t address)
{
    cartridge_mbc5 *mbc = &gb->cart->mbc->mbc5;

    uint8_t value = 0xff; // open bus
    uint16_t bankno;
    // to ignore bits beyond those needed to address ROM banks
    uint16_t rom_bitmask = (1 << gb->cart->rom_banks_bitsize) - 1;
    uint16_t ram_bitmask = (1 << gb->cart->ram_banks_bitsize) - 1;

    if (address <= 0x3fff) // ROM bank 0
        value = gb->cart->rom_banks[0][address];
    else if (address <= 0x7fff) // ROM banks 0x00-0x1ff
    {
        bankno = ((mbc->bit9_rom_bankno << 8) | mbc->lsb_rom_bankno) & rom_bitmask;
        value = gb->cart->rom_banks[bankno][address - 0x4000];
    }
    else if (0xa000 <= address && address <= 0xbfff
             && mbc->ram_enabled && gb->cart->num_ram_banks) // RAM
    {
        bankno = mbc->ram_bankno & ram_bitmask;
        value = gb->cart->ram_banks[bankno][address - 0xa000];
    }

    return value;
}

void mbc5_write(gameboy *gb, uint16_t address, uint8_t value)
{
    cartridge_mbc5 *mbc = &gb->cart->mbc->mbc5;

    if (address <= 0x1fff) // RAM enable
        mbc->ram_enabled = value == 0x0a;
    else if (address <= 0x2fff) // LSB of ROM bank number
        mbc->lsb_rom_bankno = value;
    else if (address <= 0x3fff) // bit 9 of ROM bank number
        mbc->bit9_rom_bankno = value & 1;
    else if (address <= 0x5fff) // RAM bank
        mbc->ram_bankno = value & 0x0f;
    else if (0xa000 <= address && address <= 0xbfff
             && mbc->ram_enabled && gb->cart->num_ram_banks) // RAM
    {
        uint16_t ram_bitmask = (1 << gb->cart->ram_banks_bitsize) - 1;
        uint8_t bankno = mbc->ram_bankno & ram_bitmask;
        gb->cart->ram_banks[bankno][address - 0xa000] = value;
    }
}
