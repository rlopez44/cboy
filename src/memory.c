#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "cboy/common.h"
#include "cboy/cpu.h"
#include "cboy/gameboy.h"
#include "cboy/apu.h"
#include "cboy/joypad.h"
#include "cboy/memory.h"
#include "cboy/cartridge.h"
#include "cboy/mbc.h"
#include "cboy/ppu.h"
#include "cboy/log.h"

// Determine which WRAM bank to map to the given address
static int get_wram_bank(gameboy *gb, uint16_t address)
{
    // outward-facing bank based on address
    int mmap_bank = (address >> 12) & 1;

    // translate to which physical bank is mapped
    int bankno;
    if (gb->run_mode == GB_CGB_MODE && mmap_bank)
    {
        int svbk = gb->svbk & 0x7;
        bankno = svbk ? svbk : 1;
    }
    else
    {
        bankno = mmap_bank;
    }

    return bankno;
}

uint8_t ram_read(gameboy *gb, uint16_t address)
{
    uint8_t value;
    gb_memory *mem = gb->memory;
    if (address >= 0x8000 && address <= 0x9fff)
    {
        uint16_t offset = address & 0x1fff;
        bool bankno = gb->run_mode == GB_CGB_MODE && gb->vbk & 1;
        value = mem->vram[bankno][offset];
    }
    else if (address >= 0xc000 && address <= 0xfdff)
    {
        // handle both WRAM and ECHO RAM
        int bank = get_wram_bank(gb, address);
        value = mem->wram[bank][address & 0x0fff];
    }
    else if (address >= 0xfe00 && address <= 0xfe9f)
    {
        value = mem->oam[address & 0xff];
    }
    else if (address >= 0xff80 && address <= 0xfffe)
    {
        value = mem->hram[address & 0x7f];
    }
    else
    {
        LOG_ERROR("\nExpected read from RAM address"
                  " Got: %04x\n",
                  address);
        exit(1);
    }

    return value;
}

void ram_write(gameboy *gb, uint16_t address, uint8_t value)
{
    gb_memory * mem = gb->memory;
    if (address >= 0x8000 && address <= 0x9fff)
    {
        uint16_t offset = address & 0x1fff;
        bool bankno = gb->run_mode == GB_CGB_MODE && gb->vbk & 1;
        mem->vram[bankno][offset] = value;
    }
    else if (address >= 0xc000 && address <= 0xfdff)
    {
        // handle both WRAM and ECHO RAM
        int bank = get_wram_bank(gb, address);
        mem->wram[bank][address & 0x0fff] = value;
    }
    else if (address >= 0xfe00 && address <= 0xfe9f)
    {
        mem->oam[address & 0xff] = value;
    }
    else if (address >= 0xff80 && address <= 0xfffe)
    {
        mem->hram[address & 0x7f] = value;
    }
    else
    {
        LOG_ERROR("\nExpected write to RAM address"
                  " Got: %04x\n",
                  address);
        exit(1);
    }
}

static uint8_t io_register_read(gameboy *gb, uint16_t address)
{
    uint8_t value = 0xff;
    if (address == JOYP_REGISTER)
    {
        value = report_button_states(gb);
    }
    else if (address >= DIV_REGISTER && address <= TAC_REGISTER)
    {
        value = timing_related_read(gb, address);
    }
    else if (address == IF_REGISTER)
    {
        value = interrupt_register_read(gb->cpu, IF_REGISTER);
    }
    else if (address >= NR10_REGISTER && address <= WAVE_RAM_STOP)
    {
        value = apu_read(gb, address);
    }
    else if (address >= LCDC_REGISTER && address <= WX_REGISTER)
    {
        value = ppu_read(gb, address);
    }
    else if (address == VBK_REGISTER && gb->run_mode == GB_CGB_MODE)
    {
        value = cgb_core_io_read(gb, VBK_REGISTER);
    }
    else if (address == BRD_REGISTER)
    {
        value = gb->boot_rom_disabled;
    }
    else if (address >= HDMA1_REGISTER && address <= HDMA5_REGISTER && gb->run_mode == GB_CGB_MODE)
    {
        cgb_core_io_read(gb, address);
    }
    else if (address >= BCPS_REGISTER && address <= OPRI_REGISTER && gb->run_mode == GB_CGB_MODE)
    {
        value = ppu_read(gb, address);
    }
    else if (address == SVBK_REGISTER && gb->run_mode == GB_CGB_MODE)
    {
        value = cgb_core_io_read(gb, SVBK_REGISTER);
    }
    else if (address == IE_REGISTER)
    {
        value = interrupt_register_read(gb->cpu, IE_REGISTER);
    }
    else if (address == KEY1_REGISTER && gb->run_mode == GB_CGB_MODE)
    {
        value = cgb_core_io_read(gb, KEY1_REGISTER);
    }

    return value;
}

static void io_register_write(gameboy *gb, uint16_t address, uint8_t value)
{
    if (address == JOYP_REGISTER)
    {
        update_button_set(gb, value);
    }
    else if (address >= DIV_REGISTER && address <= TAC_REGISTER)
    {
        timing_related_write(gb, address, value);
    }
    else if (address == IF_REGISTER)
    {
        interrupt_register_write(gb->cpu, IF_REGISTER, value);
    }
    else if (address >= NR10_REGISTER && address <= WAVE_RAM_STOP)
    {
        apu_write(gb, address, value);
    }
    else if (address >= LCDC_REGISTER && address <= WX_REGISTER)
    {
        ppu_write(gb, address, value);
    }
    else if (address == VBK_REGISTER && gb->run_mode == GB_CGB_MODE)
    {
        cgb_core_io_write(gb, VBK_REGISTER, value);
    }
    else if (address == BRD_REGISTER)
    {
        if (!gb->boot_rom_disabled)
            gb->boot_rom_disabled = value;
    }
    else if (address >= HDMA1_REGISTER && address <= HDMA5_REGISTER && gb->run_mode == GB_CGB_MODE)
    {
        cgb_core_io_write(gb, address, value);
    }
    else if (address >= BCPS_REGISTER && address <= OPRI_REGISTER && gb->run_mode == GB_CGB_MODE)
    {
        ppu_write(gb, address, value);
    }
    else if (address == SVBK_REGISTER && gb->run_mode == GB_CGB_MODE)
    {
        cgb_core_io_write(gb, SVBK_REGISTER, value);
    }
    else if (address == IE_REGISTER)
    {
        interrupt_register_write(gb->cpu, IE_REGISTER, value);
    }
    else if (address == KEY1_REGISTER && gb->run_mode == GB_CGB_MODE)
    {
        cgb_core_io_write(gb, KEY1_REGISTER, value);
    }
}

/********** TODO: When PPU timing emulation improves, add back OAM and VRAM blocking **********/

static bool do_access_bootrom(gameboy *gb, uint16_t address)
{
    bool addr_range1 = address < 0x100;
    bool addr_range2 = address >= 0x200 && address < 0x900;
    bool rom_enabled_and_used = gb->run_boot_rom && !gb->boot_rom_disabled;
    bool rom_addr = addr_range1 || (gb->run_mode == GB_CGB_MODE && addr_range2);

    return rom_enabled_and_used && rom_addr;
}

/* Read a byte from the Game Boy's memory map.
 * This function should only be used by the CPU.
 */
uint8_t read_byte(gameboy *gb, uint16_t address)
{
    // during a DMA transfer we can only access HRAM and the DMA register
    if (gb->dma_requested
        && (address < 0xff80 || address > 0xfffe)
        && address != DMA_REGISTER)
    {
        return 0xff;
    }

    uint8_t value;
    if (address <= 0x7fff) // cartridge ROM
    {

        if (do_access_bootrom(gb, address))
            value = gb->boot_rom[address];
        else
            value = cartridge_read(gb, address);
    }
    else if (address <= 0x9fff) // VRAM
    {
        value = ram_read(gb, address);
    }
    else if (address <= 0xbfff) // cartridge RAM
    {
        value = cartridge_read(gb, address);
    }
    else if (address <= 0xfe9f) // WRAM, ECHO RAM, OAM
    {
        value = ram_read(gb, address);
    }
    else if (address <= 0xfeff) // prohibited memory range
    {
        uint8_t ppu_status   = gb->ppu->stat & 0x3;
        bool oam_blocked  = (ppu_status == 3) || (ppu_status == 2);

        value = oam_blocked ? 0xff : 0x00;
    }
    else if (address <= 0xff7f) // I/O registers
    {
        value = io_register_read(gb, address);
    }
    else if (address <= 0xfffe) // HRAM
    {
        value = ram_read(gb, address);
    }
    else // interrupt enable register
    {
        value = io_register_read(gb, IE_REGISTER);
    }

    return value;
}

/* Write a byte to the Game Boy's memory map.
 * This function should only be used by the CPU.
 */
void write_byte(gameboy *gb, uint16_t address, uint8_t value)
{
    // during a DMA transfer we can only access HRAM and the DMA register
    if (gb->dma_requested
        && (address < 0xff80 || address > 0xfffe)
        && address != DMA_REGISTER)
    {
        return;
    }

    if (address <= 0x7fff) // cartridge ROM
    {
        cartridge_write(gb, address, value);
    }
    else if (address <= 0x9fff) // VRAM
    {
        ram_write(gb, address, value);
    }
    else if (address <= 0xbfff) // cartridge RAM
    {
        cartridge_write(gb, address, value);
    }
    else if (address <= 0xfe9f) // WRAM, ECHO RAM, OAM
    {
        ram_write(gb, address, value);
    }
    else if (address <= 0xfeff) // prohibited memory range
    {
    }
    else if (address <= 0xff7f) // I/O registers
    {
        io_register_write(gb, address, value);
    }
    else if (address <= 0xfffe) // HRAM
    {
        ram_write(gb, address, value);
    }
    else // interrupt enable register
    {
        io_register_write(gb, IE_REGISTER, value);
    }
}

/* Allocate memory for the Game Boy's internal RAM (see below)
 *
 * RAM addresses (see: https://gbdev.io/pandocs/Memory_Map.html)
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Start     End       Description
 * 0x8000    0x9fff    8KB Video RAM (VRAM)
 * 0xc000    0xcfff    4KB Work RAM (WRAM) bank 0
 * 0xd000    0xdfff    4KB Work RAM (WRAM) bank 1
 * 0xe000    0xfdff    Mirror of 0xc000-0xddff (ECHO RAM)
 * 0xfe00    0xfe9f    Sprite attribute table (OAM)
 * 0xff80    0xfffe    High RAM (HRAM)
 *
 * Returns NULL if the allocation fails.
 */
gb_memory *init_memory_map(void)
{
    gb_memory *memory = calloc(1, sizeof(gb_memory));
    return memory;
}

/* Free the allocated memory for the GB's memory map */
void free_memory_map(gb_memory *memory)
{
    free(memory);
}
