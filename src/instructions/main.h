#ifndef INSTRUCTIONS_MAIN_H
#define INSTRUCTIONS_MAIN_H

#include "cboy/instructions.h"

// load instructions
void ld(gb_cpu *cpu, gb_instruction *inst);
void ldh(gb_cpu *cpu, gb_instruction *inst);

// arithmetic instructions
void inc(gb_cpu *cpu, gb_instruction *inst);

#endif
