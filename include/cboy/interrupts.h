#ifndef GB_INTERRUPTS_H
#define GB_INTERRUPTS_H

#include <stdbool.h>
#include "cboy/gameboy.h"

// The five interrupt types. These interrupts are listed
// such that their value is also their appropriate bit in IF
// and IE registers
typedef enum INTERRUPT_TYPE {
    VBLANK,
    LCD_STAT,
    TIMER,
    SERIAL,
    JOYPAD,
} INTERRUPT_TYPE;

// request an interrupt by setting the appropriate bit in the IF register
void request_interrupt(gameboy *gb, INTERRUPT_TYPE interrupt);

// set the appropriate bit in the IE register to enable the given interrupt
void enable_interrupt(gameboy *gb, INTERRUPT_TYPE interrupt);

// handle an interrupt, if any needs to be handled
uint8_t service_interrupt(gameboy *gb);

// returns set bits for all interrupts that are both pending and enabled
uint8_t pending_interrupts(gameboy *gb);

#endif
