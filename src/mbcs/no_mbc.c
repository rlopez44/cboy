#include "dispatch.h"
#include "cboy/log.h"

uint8_t no_mbc_read(gameboy *gb, uint16_t address)
{
    uint8_t value = 0xff; // open bus value

    if (address <= 0x3fff) // ROM bank 0
        value = gb->cart->rom_banks[0][address];
    else if (address <= 0x7fff) // ROM bank 1
        value = gb->cart->rom_banks[1][address - 0x4000];
    else if (0xa000 <= address && address <= 0xbfff) // RAM
    {
        // cartridge has 0 or 1 RAM banks
        if (gb->cart->num_ram_banks)
            value = gb->cart->ram_banks[0][address - 0xa000];
    }

    return value;
}

void no_mbc_write(gameboy *gb, uint16_t address, uint8_t value)
{
    // only allow writes to RAM
    if (0xa000 <= address && address <= 0xbfff)
    {
        // 0 or 1 RAM banks
        if (gb->cart->num_ram_banks)
            gb->cart->ram_banks[0][address - 0xa000] = value;
    }
}
