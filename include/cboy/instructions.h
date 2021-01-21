#ifndef INSTRUCTIONS_H_
#define INSTRUCTIONS_H_

#include <stdint.h>
#include "cboy/cpu.h"

// Game Boy CPU opcodes
// See: https://gbdev.io/gb-opcodes/optables/
enum opcodes {
    NOP,
    LD,
    INC,
    DEC,
    RLCA,
    ADD,
    RRCA,
    STOP,
    RLA,
    JR,
    RRA,
    DAA,
    CPL,
    SCF,
    CCF,
    HALT,
    ADC,
    SUB,
    SBC,
    AND,
    XOR,
    OR,
    CP,
    RET,
    POP,
    JP,
    CALL,
    PUSH,
    RST,
    PREFIX,
    UNUSED,
    RETI,
    LDH,
    DI,
    EI,
    // CB Opcodes
    RLC,
    RRC,
    RL,
    RR,
    SLA,
    SRA,
    SWAP,
    SRL,
    BIT,
    RES,
    SET,
};

// The operands that may be used with the CPU's opcodes
enum operands {
    NONE, // to represent 'no operand'
    // registers
    REG_A,
    REG_B,
    REG_C,
    REG_D,
    REG_E,
    REG_F,
    REG_H,
    REG_L,
    REG_AF,
    REG_BC,
    REG_DE,
    REG_HL,
    REG_SP,
    // 8-bit and 16-bit immediate values
    // NOTE: 8-bit immediate may be signed or unsigned based on opcode
    IMM_8,
    IMM_16,
    // 8-bit 'address' (added to 0xFF00 to determine 16-bit address)
    PTR_8,
    // 16-bit address
    PTR_16,
    // register values as pointers
    PTR_C,
    PTR_BC,
    PTR_DE,
    PTR_HL,
    PTR_HL_DEC, // decrement value of HL after executing instruction
    PTR_HL_INC, // increment value of HL after executing instruction
    // condition codes
    CC_Z,
    CC_NZ,
    CC_C,
    CC_NC,
    // RST vectors
    PTR_0x00,
    PTR_0x08,
    PTR_0x10,
    PTR_0x18,
    PTR_0x20,
    PTR_0x28,
    PTR_0x30,
    PTR_0x38,
    // bit numbers for bit instructions
    BIT_0,
    BIT_1,
    BIT_2,
    BIT_3,
    BIT_4,
    BIT_5,
    BIT_6,
    BIT_7,
};


typedef struct gb_instruction {
    enum opcodes opcode;
    enum operands op1, op2;
    uint8_t length; // in bytes
    // alt duration only used for conditional calls and returns
    uint8_t duration, alt_duration; // in M-cycles (one M-cycle = four clock ticks)
} gb_instruction;

// executes the cpu instruction specified by the PC
// returns the number of m-cycles elapsed during instruction execution
uint8_t execute_instruction(gb_cpu *cpu);

#endif
