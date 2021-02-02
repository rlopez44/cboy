#include <stdint.h>
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
