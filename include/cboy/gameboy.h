#ifndef GAME_BOY_H
#define GAME_BOY_H

#include <stdbool.h>
#include <stdint.h>
#include "cboy/cpu.h"
#include "cboy/memory.h"

typedef struct gameboy {
    gb_cpu *cpu;
    gb_memory *memory;

    bool is_stopped, is_halted;
} gameboy;

// stack push and pop operations
void stack_push(gameboy *gb, uint16_t value);

uint16_t stack_pop(gameboy *gb);

#endif
