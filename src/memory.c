#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "cboy/memory.h"
#include "cboy/cartridge.h"

/* for now simply read and write to the byte at the given memory address.
 * as more components of the emulator are implemented, these functions
 * will likely become more complex.
 */
uint8_t read_byte(gb_memory *memory, uint16_t address)
{
    return (memory->memory)[address];
}

void write_byte(gb_memory *memory, uint16_t address, uint8_t value)
{
    (memory->memory)[address] = value;
}

/* Initialize the I/O registers of the Game Boy
 * whose initial value is non-zero.
 *
 * See: https://gbdev.io/pandocs/#power-up-sequence
 */
static void init_io_registers(gb_memory *memory)
{
    write_byte(memory, 0xff10, 0x80); // NR10
    write_byte(memory, 0xff11, 0xbf); // NR11
    write_byte(memory, 0xff12, 0xf3); // NR12
    write_byte(memory, 0xff14, 0xbf); // NR14
    write_byte(memory, 0xff16, 0x3f); // NR21
    write_byte(memory, 0xff19, 0xbf); // NR24
    write_byte(memory, 0xff1a, 0x7f); // NR30
    write_byte(memory, 0xff1b, 0xff); // NR31
    write_byte(memory, 0xff1c, 0x9f); // NR32
    write_byte(memory, 0xff1e, 0xbf); // NR34
    write_byte(memory, 0xff20, 0xff); // NR41
    write_byte(memory, 0xff23, 0xbf); // NR44
    write_byte(memory, 0xff24, 0x77); // NR50
    write_byte(memory, 0xff25, 0xf3); // NR51
    write_byte(memory, 0xff26, 0xf1); // NR52
    write_byte(memory, 0xff40, 0x91); // LCDC
    write_byte(memory, 0xff47, 0xfc); // BGP
    write_byte(memory, 0xff48, 0xff); // OBP0
    write_byte(memory, 0xff49, 0xff); // OBP1
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
        memory->memory[i] = 0;
    }

    // disable the boot ROM
    // TODO: make this byte unchangeable
    write_byte(memory, 0xff50, 1);

    // mount the zeroth and first ROM banks
    memcpy(memory->memory, cart->rom_banks[0], ROM_BANK_SIZE * sizeof(uint8_t));
    memcpy(memory->memory + ROM_BANK_SIZE, cart->rom_banks[1], ROM_BANK_SIZE * sizeof(uint8_t));

    init_io_registers(memory);

    return memory;
}

/* Free the allocated memory for the GB's memory map */
void free_memory_map(gb_memory *memory)
{
    free(memory);
}
