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
uint8_t ret(gameboy *gb, gb_instruction *inst);
void reti(gameboy *gb);

// bit shift instructions
void rrca(gameboy *gb);
void rlca(gameboy *gb);
void rra(gameboy *gb);
void rla(gameboy *gb);
void rlc(gameboy *gb, gb_instruction *inst);
void rrc(gameboy *gb, gb_instruction *inst);
void rl(gameboy *gb, gb_instruction *inst);
void rr(gameboy *gb, gb_instruction *inst);
void sla(gameboy *gb, gb_instruction *inst);
void sra(gameboy *gb, gb_instruction *inst);
void srl(gameboy *gb, gb_instruction *inst);
void swap(gameboy *gb, gb_instruction *inst);

// miscellaneous instructions
void ei(gameboy *gb);
void di(gameboy *gb);
void push(gameboy *gb, gb_instruction *inst);
void pop(gameboy *gb, gb_instruction *inst);
void daa(gameboy *gb);
void scf(gameboy *gb);
void ccf(gameboy *gb);
void cpl(gameboy *gb);
void stop(gameboy *gb);
void halt(gameboy *gb);

#endif
