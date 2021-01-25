#ifndef INSTRUCTIONS_EXECUTE_H
#define INSTRUCTIONS_EXECUTE_H

#include "cboy/instructions.h"

// load instructions
void ld(gb_cpu *cpu, gb_instruction *inst);
void ldh(gb_cpu *cpu, gb_instruction *inst);

// arithmetic instructions
void inc(gb_cpu *cpu, gb_instruction *inst);
void dec(gb_cpu *cpu, gb_instruction *inst);
void add(gb_cpu *cpu, gb_instruction *inst);
void adc(gb_cpu *cpu, gb_instruction *inst);
void sub(gb_cpu *cpu, gb_instruction *inst);
void sbc(gb_cpu *cpu, gb_instruction *inst);
void and(gb_cpu *cpu, gb_instruction *inst);
void or(gb_cpu *cpu, gb_instruction *inst);
void xor(gb_cpu *cpu, gb_instruction *inst);

#endif
