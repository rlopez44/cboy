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

#endif
