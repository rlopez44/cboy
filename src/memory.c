#include <stdint.h>
#include <stdlib.h>
#include "cboy/memory.h"

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

/* Allocate memory for the Game Boy's memory
 * map and initialize it to zero.
 *
 * NOTE: the byte at memory address 0xff50 is
 * used to disable the Game Boy's internal
 * boot ROM. Setting this byte to 1 disables
 * the boot ROM and allows the cartridge
 * to take over. Because we will start
 * the emulator in the state right after
 * the boot ROM has executed, we will set
 * this byte to 1. Once this byte has been
 * set to 1 it cannot be changed and can
 * only be reset by resetting the Game Boy.
 *
 * Returns NULL if the allocation fails.
 */
gb_memory *init_memory_map(void)
{
    gb_memory *memory = malloc(sizeof(gb_memory));

    if (memory == NULL)
    {
        return NULL;
    }

    // init all array values to 0
    for (int i = 0; i < MEMORY_MAP_SIZE; ++i)
    {
        (memory->memory)[i] = 0;
    }

    // disable the boot ROM
    // TODO: make this byte unchangeable
    write_byte(memory, 0xff50, 1);

    return memory;
}

/* Free the allocated memory for the GB's memory map */
void free_memory_map(gb_memory *memory)
{
    free(memory);
}
