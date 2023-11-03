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

 /********** TODO: When PPU timing emulation improves, add back OAM and VRAM blocking **********/

/* Reads a byte from the Game Boy's memory map at the given
 * address. Because a read can be requested from any address,
 * we have to implement certain checks on the address so that
 * the developer is not allowed to perform illegal reads.
 * See below.
 *
 * Constraints on memory reads
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *  > Echo RAM: reads from the memory range 0xe000-0xfdff are
 *    mapped to 0xc000-0xddff (an area of WRAM). Nintendo
 *    prohibits the use of this memory range by developers.
 *
 *  > Reads from the memory range 0xfea0-0xfeff are prohibited
 *    by Nintendo.
 *      >> 0xff is returned when OAM is blocked
 *         Otherwise, 0x00 is returned.
 */
uint8_t read_byte(gameboy *gb, uint16_t address)
{
    uint8_t ppu_status   = gb->ppu->stat & 0x3,
            oam_blocked  = (ppu_status == 3) || (ppu_status == 2);

    /**************** BEGIN: Special reads where we return early **************/
    // a boot ROM was successfully loaded
    if (gb->run_boot_rom && address < 0x100 && !gb->boot_rom_disabled)
    {
        return gb->boot_rom[address];
    }
    // during a DMA transfer we can only access HRAM and the DMA register
    else if (gb->dma_requested
             && (address < 0xff80 || address > 0xfffe)
             && address != DMA_REGISTER)
    {
        return 0xff;
    }
    // attempted read from the prohibited memory range
    else if (0xfea0 <= address && address <= 0xfeff)
    {
        return oam_blocked ? 0xff : 0x00;
    }
    else if (address == DIV_REGISTER
             || address == TIMA_REGISTER
             || address == TMA_REGISTER
             || address == TAC_REGISTER)
    {
        return timing_related_read(gb, address);
    }
    else if (address >= LCDC_REGISTER && address <= WX_REGISTER)
    {
        return ppu_read(gb, address);
    }
    else if (address == JOYP_REGISTER)
        return report_button_states(gb);

    else if ((address <= 0x7fff) // cartridge ROM
             || (0xa000 <= address && address <= 0xbfff)) // cartridge RAM
    {
        return cartridge_read(gb, address);
    }
    else if (address >= 0xff10 && address <= 0xff3f) // APU registers + wave RAM
    {
        return apu_read(gb, address);
    }
    else if (address == BRD_REGISTER)
    {
        return gb->boot_rom_disabled;
    }
    else if (address == IF_REGISTER || address == IE_REGISTER)
    {
        return interrupt_register_read(gb->cpu, address);
    }
    /**************** END: Special reads where we return early **************/


    // attempted read from Echo RAM
    if (0xe000 <= address && address <= 0xfdff)
    {
        // map to the appropriate address in WRAM
        // the offset is 0xe000 - 0xc000 = 0x2000
        address -= 0x2000;
    }

    return gb->memory->mmap[address];
}

/* Perform a DMA transfer from ROM or RAM to OAM
 * ---------------------------------------------
 * On hardware, the DMA transfer takes 160 m-cycles to complete,
 * but this function performs the transfer all at once. As such,
 * emulating the timing of the transfer should be handled by the
 * caller.
 *
 * The DMA source address is really the upper byte of the full
 * 16-bit starting address for the transfer (See below).
 *
 * Source and Destination
 * ~~~~~~~~~~~~~~~~~~~~~~
 * DMA source address: XX
 * Source:          0xXX00 - 0xXX9f
 * Destination:     0xfe00 - 0xfe9f
 *
 */
void dma_transfer(gameboy *gb)
{
    uint8_t source_hi = gb->ppu->dma;
    uint16_t source, dest;
    bool mbc_read;

    for (uint16_t lo = 0x0000; lo <= 0x009f; ++lo)
    {
        source = ((uint16_t)source_hi << 8) | lo;
        dest = 0xfe00 | lo;
        mbc_read = source <= 0x7fff || (0xa000 <= source && source <= 0xbfff);

        // source may be in cartridge RAM/ROM
        if (mbc_read)
            gb->memory->mmap[dest] = cartridge_read(gb, source);
        else
            gb->memory->mmap[dest] = gb->memory->mmap[source];
    }
}

/* Writes the given value to the byte at the given address in the
 * Game Boy's memory map. Because writes can be requested to any
 * address, we have to implement checks on the address to make sure
 * the developer is not allowed to perform illegal writes.
 * See below.
 *
 * Constraints on memory reads
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *  > Address 0xff50 contains the boot ROM disable bit. This bit
 *    is set to 1 when the Game Boy's boot ROM finishes executing,
 *    and can only be reset by restarting the Game Boy. Because this
 *    emulator's initial state is that of the Game Boy after the boot
 *    ROM has finished executing, the boot ROM disable bit will be set
 *    and will not be allowed to change.
 *
 *  > Writes to the ROM area of the memory map (0x0000-0x7fff) are
 *    intercepted by the cartridge's Memory Bank Controller, if any.
 *    TODO: add support for MBCs.
 *
 *  > Echo RAM: writes to the memory range 0xe000-0xfdff are mapped
 *    to 0xc000-0xddff (an area of WRAM). Nintendo prohibits the use
 *    of this memory range by developers.
 *
 *  > Writes to the memory range 0xfea0-0xfeff are prohibited by
 *    Nintendo.
 */
void write_byte(gameboy *gb, uint16_t address, uint8_t value)
{
    bool prohibited_mem_range = 0xfea0 <= address && address <= 0xfeff;

    // during a DMA transfer we can only access HRAM and the DMA register
    bool dma_block = gb->dma_requested
                     && (address < 0xff80 || address > 0xfffe)
                     && address != DMA_REGISTER;

    // attempted write to the boot ROM disabled bit
    // after the boot ROM has finished running
    bool illegal_boot_rom_enable_write = gb->boot_rom_disabled && address == BRD_REGISTER;
    // LY register is read-only
    bool ly_write = address == LY_REGISTER;

    /**************** BEGIN: Special writes where we return early **************/
    if (dma_block || illegal_boot_rom_enable_write
        || prohibited_mem_range || ly_write)
    {
        return;
    }
    // writing to the timing-related registers
    else if (0xff04 <= address && address <= 0xff07)
    {
        timing_related_write(gb, address, value);
        return;
    }
    else if ((address <= 0x7fff) // cartridge ROM
             || (0xa000 <= address && address <= 0xbfff)) // cartridge RAM
    {
        cartridge_write(gb, address, value);
        return;
    }
    else if (address >= 0xff10 && address <= 0xff3f) // APU registers + wave RAM
    {
        apu_write(gb, address, value);
        return;
    }
    else if (address >= LCDC_REGISTER && address <= WX_REGISTER)
    {
        ppu_write(gb, address, value);
        return;
    }
    else if (address == BRD_REGISTER)
    {
        gb->boot_rom_disabled = value;
        return;
    }
    // until double speed mode implemented: return early
    else if (address == KEY1_REGISTER && gb->run_mode == GB_CGB_MODE)
    {
        LOG_INFO("\nNote: Speed switching not implemented."
                 " Continuing to run in normal speed mode\n");

        return;
    }
    // writes to JOYP determine which set of buttons are reported
    else if (address == JOYP_REGISTER)
    {
        update_button_set(gb, value);
        return;
    }
    else if (address == IF_REGISTER || address == IE_REGISTER)
    {
        interrupt_register_write(gb->cpu, address, value);
        return;
    }
    /**************** END: Special writes where we return early **************/


    // attempted write to Echo RAM
    if (0xe000 <= address && address <= 0xfdff)
    {
        // map to the appropriate address in WRAM
        // the offset is 0xe000 - 0xc000 = 0x2000
        address -= 0x2000;
    }

    gb->memory->mmap[address] = value;
}

/* Allocate memory for the Game Boy's memory map and
 * initialize it to zero. Then, mount the zeroth and first
 * ROM banks from the cartridge into the appropriate locations
 * (see Memory Map below). Lastly, initialize the I/O registers.
 *
 * Memory Map (see: https://gbdev.io/pandocs/#memory-map)
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Start     End       Description
 * 0x0000    0x3fff    16KB ROM bank 00
 * 0x4000    0x7fff    16KB ROM bank 01-NN, switchable via MBC, if any
 * 0x8000    0x9fff    8KB Video RAM (VRAM)
 * 0xa000    0xbfff    8KB External RAM, in cartridge, switchable bank if any
 * 0xc000    0xcfff    4KB Work RAM (WRAM) bank 0
 * 0xd000    0xdfff    4KB Work RAM (WRAM) bank 1
 * 0xe000    0xfdff    Mirror of 0xc000-0xddff (ECHO RAM)
 * 0xfe00    0xfe9f    Sprite attribute table (OAM)
 * 0xfea0    0xfeff    Not Usable
 * 0xff00    0xff7f    I/O Registers
 * 0xff80    0xfffe    High RAM (HRAM)
 * 0xffff    0xffff    Interrupts Enable Register (IE)
 *
 *
 * NOTE: the byte at memory address 0xff50 is used to disable
 * the Game Boy's internal boot ROM. Setting this byte to 1
 * disables the boot ROM and allows the cartridge to take over.
 * Because we will start the emulator in the state right after
 * the boot ROM has executed, we will set this byte to 1. Once
 * this byte has been set to 1 it cannot be changed and can
 * only be reset by resetting the Game Boy.
 *
 * Returns NULL if the allocation fails.
 */
gb_memory *init_memory_map(gb_cartridge *cart)
{
    gb_memory *memory = malloc(sizeof(gb_memory));

    if (memory == NULL)
    {
        return NULL;
    }

    // init all array values to 0
    for (int i = 0; i < MEMORY_MAP_SIZE; ++i)
    {
        memory->mmap[i] = 0;
    }

    // mount the zeroth and first ROM banks
    memcpy(memory->mmap, cart->rom_banks[0], ROM_BANK_SIZE);
    memcpy(memory->mmap + ROM_BANK_SIZE, cart->rom_banks[1], ROM_BANK_SIZE);

    return memory;
}

/* Free the allocated memory for the GB's memory map */
void free_memory_map(gb_memory *memory)
{
    free(memory);
}
