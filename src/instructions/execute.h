#ifndef INSTRUCTIONS_EXECUTE_H
#define INSTRUCTIONS_EXECUTE_H

#include "cboy/gameboy.h"
#include "cboy/instructions.h"

// load instructions
void ld(gameboy *gb, gb_instruction *inst);
void ldh(gameboy *gb, gb_instruction *inst);

// arithmetic instructions
void inc(gameboy *gb, gb_instruction *inst);
void dec(gameboy *gb, gb_instruction *inst);
void add(gameboy *gb, gb_instruction *inst);
void adc(gameboy *gb, gb_instruction *inst);
void sub(gameboy *gb, gb_instruction *inst);
void sbc(gameboy *gb, gb_instruction *inst);
void cp(gameboy *gb, gb_instruction *inst);
void and(gameboy *gb, gb_instruction *inst);
void or(gameboy *gb, gb_instruction *inst);
void xor(gameboy *gb, gb_instruction *inst);

// jump and subroutine-related instructions
uint8_t jp(gameboy *gb, gb_instruction *inst);
uint8_t jr(gameboy *gb, gb_instruction *inst);
uint8_t call(gameboy *gb, gb_instruction *inst);
void rst(gameboy *gb, gb_instruction *inst);

#endif
