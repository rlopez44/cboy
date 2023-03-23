#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "cboy/instructions.h"
#include "cboy/gameboy.h"
#include "cboy/interrupts.h"
#include "cboy/memory.h"
#include "cboy/log.h"
#include "execute.h"

// string representations of the CPU opcode operands
// these representations can be indexed using the operands enum
const char *operand_strs[NUM_OPERANDS] = {
    "", // no operand
    // registers
    "A",
    "B",
    "C",
    "D",
    "E",
    "F",
    "H",
    "L",
    "AF",
    "BC",
    "DE",
    "HL",
    "SP",
    // 8-bit and 16-bit immediate values
    "n8",
    "n16",
    // 8-bit 'address'
    "[a8]",
    // 16-bit address
    "[a16]",
    // register values as pointers
    "[C]",
    "[BC]",
    "[DE]",
    "[HL]",
    "[HL-]", // decrement value of HL after executing instruction
    "[HL+]", // increment value of HL after executing instruction
    // condition codes
    "Z",
    "NZ",
    "C",
    "NC",
    // RST vectors
    "00H",
    "08H",
    "10H",
    "18H",
    "20H",
    "28H",
    "30H",
    "38H",
    // bit numbers for bit instructions
    "0",
    "1",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
};

// the CPU's instruction set
static const gb_instruction instruction_table[512] = {
    /* FORMAT (durations specified in M-cycles)
     * ------
     * [NUMERIC_CODE] = {OPCODE, OP1, OP2, LENGTH, DURATION, ALT_DURATION, STRING_REPR},
     */

    // low 256 instructions
    [0x00] = {NOP, NONE, NONE, 1, 1, 1, "NOP"},
    [0x01] = {LD, REG_BC, IMM_16, 3, 3, 3, "LD"},
    [0x02] = {LD, PTR_BC, REG_A, 1, 2, 2, "LD"},
    [0x03] = {INC, REG_BC, NONE, 1, 2, 2, "INC"},
    [0x04] = {INC, REG_B, NONE, 1, 1, 1, "INC"},
    [0x05] = {DEC, REG_B, NONE, 1, 1, 1, "DEC"},
    [0x06] = {LD, REG_B, IMM_8, 2, 2, 2, "LD"},
    [0x07] = {RLCA, REG_A, NONE, 1, 1, 1, "RLCA"},
    [0x08] = {LD, PTR_16, REG_SP, 3, 5, 5, "LD"},
    [0x09] = {ADD, REG_HL, REG_BC, 1, 2, 2, "ADD"},
    [0x0a] = {LD, REG_A, PTR_BC, 1, 2, 2, "LD"},
    [0x0b] = {DEC, REG_BC, NONE, 1, 2, 2, "DEC"},
    [0x0c] = {INC, REG_C, NONE, 1, 1, 1, "INC"},
    [0x0d] = {DEC, REG_C, NONE, 1, 1, 1, "DEC"},
    [0x0e] = {LD, REG_C, IMM_8, 2, 2, 2, "LD"},
    [0x0f] = {RRCA, REG_A, NONE, 1, 1, 1, "RRCA"},
    [0x10] = {STOP, NONE, NONE, 2, 1, 1, "STOP"},
    [0x11] = {LD, REG_DE, IMM_16, 3, 3, 3, "LD"},
    [0x12] = {LD, PTR_DE, REG_A, 1, 2, 2, "LD"},
    [0x13] = {INC, REG_DE, NONE, 1, 2, 2, "INC"},
    [0x14] = {INC, REG_D, NONE, 1, 1, 1, "INC"},
    [0x15] = {DEC, REG_D, NONE, 1, 1, 1, "DEC"},
    [0x16] = {LD, REG_D, IMM_8, 2, 2, 2, "LD"},
    [0x17] = {RLA, REG_A, NONE, 1, 1, 1, "RLA"},
    [0x18] = {JR, IMM_8, NONE, 2, 3, 3, "JR"},
    [0x19] = {ADD, REG_HL, REG_DE, 1, 2, 2, "ADD"},
    [0x1a] = {LD, REG_A, PTR_DE, 1, 2, 2, "LD"},
    [0x1b] = {DEC, REG_DE, NONE, 1, 2, 2, "DEC"},
    [0x1c] = {INC, REG_E, NONE, 1, 1, 1, "INC"},
    [0x1d] = {DEC, REG_E, NONE, 1, 1, 1, "DEC"},
    [0x1e] = {LD, REG_E, IMM_8, 2, 2, 2, "LD"},
    [0x1f] = {RRA, REG_A, NONE, 1, 1, 1, "RRA"},
    [0x20] = {JR, CC_NZ, IMM_8, 2, 3, 2, "JR"},
    [0x21] = {LD, REG_HL, IMM_16, 3, 3, 3, "LD"},
    [0x22] = {LD, PTR_HL_INC, REG_A, 1, 2, 2, "LD"},
    [0x23] = {INC, REG_HL, NONE, 1, 2, 2, "INC"},
    [0x24] = {INC, REG_H, NONE, 1, 1, 1, "INC"},
    [0x25] = {DEC, REG_H, NONE, 1, 1, 1, "DEC"},
    [0x26] = {LD, REG_H, IMM_8, 2, 2, 2, "LD"},
    [0x27] = {DAA, NONE, NONE, 1, 1, 1, "DAA"},
    [0x28] = {JR, CC_Z, IMM_8, 2, 3, 2, "JR"},
    [0x29] = {ADD, REG_HL, REG_HL, 1, 2, 2, "ADD"},
    [0x2a] = {LD, REG_A, PTR_HL_INC, 1, 2, 2, "LD"},
    [0x2b] = {DEC, REG_HL, NONE, 1, 2, 2, "DEC"},
    [0x2c] = {INC, REG_L, NONE, 1, 1, 1, "INC"},
    [0x2d] = {DEC, REG_L, NONE, 1, 1, 1, "DEC"},
    [0x2e] = {LD, REG_L, IMM_8, 2, 2, 2, "LD"},
    [0x2f] = {CPL, REG_A, NONE, 1, 1, 1, "CPL"},
    [0x30] = {JR, CC_NC, IMM_8, 2, 3, 2, "JR"},
    [0x31] = {LD, REG_SP, IMM_16, 3, 3, 3, "LD"},
    [0x32] = {LD, PTR_HL_DEC, REG_A, 1, 2, 2, "LD"},
    [0x33] = {INC, REG_SP, NONE, 1, 2, 2, "INC"},
    [0x34] = {INC, PTR_HL, NONE, 1, 3, 3, "INC"},
    [0x35] = {DEC, PTR_HL, NONE, 1, 3, 3, "DEC"},
    [0x36] = {LD, PTR_HL, IMM_8, 2, 3, 3, "LD"},
    [0x37] = {SCF, NONE, NONE, 1, 1, 1, "SCF"},
    [0x38] = {JR, CC_C, IMM_8, 2, 3, 2, "JR"},
    [0x39] = {ADD, REG_HL, REG_SP, 1, 2, 2, "ADD"},
    [0x3a] = {LD, REG_A, PTR_HL_DEC, 1, 2, 2, "LD"},
    [0x3b] = {DEC, REG_SP, NONE, 1, 2, 2, "DEC"},
    [0x3c] = {INC, REG_A, NONE, 1, 1, 1, "INC"},
    [0x3d] = {DEC, REG_A, NONE, 1, 1, 1, "DEC"},
    [0x3e] = {LD, REG_A, IMM_8, 2, 2, 2, "LD"},
    [0x3f] = {CCF, NONE, NONE, 1, 1, 1, "CCF"},
    [0x40] = {LD, REG_B, REG_B, 1, 1, 1, "LD"},
    [0x41] = {LD, REG_B, REG_C, 1, 1, 1, "LD"},
    [0x42] = {LD, REG_B, REG_D, 1, 1, 1, "LD"},
    [0x43] = {LD, REG_B, REG_E, 1, 1, 1, "LD"},
    [0x44] = {LD, REG_B, REG_H, 1, 1, 1, "LD"},
    [0x45] = {LD, REG_B, REG_L, 1, 1, 1, "LD"},
    [0x46] = {LD, REG_B, PTR_HL, 1, 2, 2, "LD"},
    [0x47] = {LD, REG_B, REG_A, 1, 1, 1, "LD"},
    [0x48] = {LD, REG_C, REG_B, 1, 1, 1, "LD"},
    [0x49] = {LD, REG_C, REG_C, 1, 1, 1, "LD"},
    [0x4a] = {LD, REG_C, REG_D, 1, 1, 1, "LD"},
    [0x4b] = {LD, REG_C, REG_E, 1, 1, 1, "LD"},
    [0x4c] = {LD, REG_C, REG_H, 1, 1, 1, "LD"},
    [0x4d] = {LD, REG_C, REG_L, 1, 1, 1, "LD"},
    [0x4e] = {LD, REG_C, PTR_HL, 1, 2, 2, "LD"},
    [0x4f] = {LD, REG_C, REG_A, 1, 1, 1, "LD"},
    [0x50] = {LD, REG_D, REG_B, 1, 1, 1, "LD"},
    [0x51] = {LD, REG_D, REG_C, 1, 1, 1, "LD"},
    [0x52] = {LD, REG_D, REG_D, 1, 1, 1, "LD"},
    [0x53] = {LD, REG_D, REG_E, 1, 1, 1, "LD"},
    [0x54] = {LD, REG_D, REG_H, 1, 1, 1, "LD"},
    [0x55] = {LD, REG_D, REG_L, 1, 1, 1, "LD"},
    [0x56] = {LD, REG_D, PTR_HL, 1, 2, 2, "LD"},
    [0x57] = {LD, REG_D, REG_A, 1, 1, 1, "LD"},
    [0x58] = {LD, REG_E, REG_B, 1, 1, 1, "LD"},
    [0x59] = {LD, REG_E, REG_C, 1, 1, 1, "LD"},
    [0x5a] = {LD, REG_E, REG_D, 1, 1, 1, "LD"},
    [0x5b] = {LD, REG_E, REG_E, 1, 1, 1, "LD"},
    [0x5c] = {LD, REG_E, REG_H, 1, 1, 1, "LD"},
    [0x5d] = {LD, REG_E, REG_L, 1, 1, 1, "LD"},
    [0x5e] = {LD, REG_E, PTR_HL, 1, 2, 2, "LD"},
    [0x5f] = {LD, REG_E, REG_A, 1, 1, 1, "LD"},
    [0x60] = {LD, REG_H, REG_B, 1, 1, 1, "LD"},
    [0x61] = {LD, REG_H, REG_C, 1, 1, 1, "LD"},
    [0x62] = {LD, REG_H, REG_D, 1, 1, 1, "LD"},
    [0x63] = {LD, REG_H, REG_E, 1, 1, 1, "LD"},
    [0x64] = {LD, REG_H, REG_H, 1, 1, 1, "LD"},
    [0x65] = {LD, REG_H, REG_L, 1, 1, 1, "LD"},
    [0x66] = {LD, REG_H, PTR_HL, 1, 2, 2, "LD"},
    [0x67] = {LD, REG_H, REG_A, 1, 1, 1, "LD"},
    [0x68] = {LD, REG_L, REG_B, 1, 1, 1, "LD"},
    [0x69] = {LD, REG_L, REG_C, 1, 1, 1, "LD"},
    [0x6a] = {LD, REG_L, REG_D, 1, 1, 1, "LD"},
    [0x6b] = {LD, REG_L, REG_E, 1, 1, 1, "LD"},
    [0x6c] = {LD, REG_L, REG_H, 1, 1, 1, "LD"},
    [0x6d] = {LD, REG_L, REG_L, 1, 1, 1, "LD"},
    [0x6e] = {LD, REG_L, PTR_HL, 1, 2, 2, "LD"},
    [0x6f] = {LD, REG_L, REG_A, 1, 1, 1, "LD"},
    [0x70] = {LD, PTR_HL, REG_B, 1, 2, 2, "LD"},
    [0x71] = {LD, PTR_HL, REG_C, 1, 2, 2, "LD"},
    [0x72] = {LD, PTR_HL, REG_D, 1, 2, 2, "LD"},
    [0x73] = {LD, PTR_HL, REG_E, 1, 2, 2, "LD"},
    [0x74] = {LD, PTR_HL, REG_H, 1, 2, 2, "LD"},
    [0x75] = {LD, PTR_HL, REG_L, 1, 2, 2, "LD"},
    [0x76] = {HALT, NONE, NONE, 1, 1, 1, "HALT"},
    [0x77] = {LD, PTR_HL, REG_A, 1, 2, 2, "LD"},
    [0x78] = {LD, REG_A, REG_B, 1, 1, 1, "LD"},
    [0x79] = {LD, REG_A, REG_C, 1, 1, 1, "LD"},
    [0x7a] = {LD, REG_A, REG_D, 1, 1, 1, "LD"},
    [0x7b] = {LD, REG_A, REG_E, 1, 1, 1, "LD"},
    [0x7c] = {LD, REG_A, REG_H, 1, 1, 1, "LD"},
    [0x7d] = {LD, REG_A, REG_L, 1, 1, 1, "LD"},
    [0x7e] = {LD, REG_A, PTR_HL, 1, 2, 2, "LD"},
    [0x7f] = {LD, REG_A, REG_A, 1, 1, 1, "LD"},
    [0x80] = {ADD, REG_A, REG_B, 1, 1, 1, "ADD"},
    [0x81] = {ADD, REG_A, REG_C, 1, 1, 1, "ADD"},
    [0x82] = {ADD, REG_A, REG_D, 1, 1, 1, "ADD"},
    [0x83] = {ADD, REG_A, REG_E, 1, 1, 1, "ADD"},
    [0x84] = {ADD, REG_A, REG_H, 1, 1, 1, "ADD"},
    [0x85] = {ADD, REG_A, REG_L, 1, 1, 1, "ADD"},
    [0x86] = {ADD, REG_A, PTR_HL, 1, 2, 2, "ADD"},
    [0x87] = {ADD, REG_A, REG_A, 1, 1, 1, "ADD"},
    [0x88] = {ADC, REG_A, REG_B, 1, 1, 1, "ADC"},
    [0x89] = {ADC, REG_A, REG_C, 1, 1, 1, "ADC"},
    [0x8a] = {ADC, REG_A, REG_D, 1, 1, 1, "ADC"},
    [0x8b] = {ADC, REG_A, REG_E, 1, 1, 1, "ADC"},
    [0x8c] = {ADC, REG_A, REG_H, 1, 1, 1, "ADC"},
    [0x8d] = {ADC, REG_A, REG_L, 1, 1, 1, "ADC"},
    [0x8e] = {ADC, REG_A, PTR_HL, 1, 2, 2, "ADC"},
    [0x8f] = {ADC, REG_A, REG_A, 1, 1, 1, "ADC"},
    [0x90] = {SUB, REG_A, REG_B, 1, 1, 1, "SUB"},
    [0x91] = {SUB, REG_A, REG_C, 1, 1, 1, "SUB"},
    [0x92] = {SUB, REG_A, REG_D, 1, 1, 1, "SUB"},
    [0x93] = {SUB, REG_A, REG_E, 1, 1, 1, "SUB"},
    [0x94] = {SUB, REG_A, REG_H, 1, 1, 1, "SUB"},
    [0x95] = {SUB, REG_A, REG_L, 1, 1, 1, "SUB"},
    [0x96] = {SUB, REG_A, PTR_HL, 1, 2, 2, "SUB"},
    [0x97] = {SUB, REG_A, REG_A, 1, 1, 1, "SUB"},
    [0x98] = {SBC, REG_A, REG_B, 1, 1, 1, "SBC"},
    [0x99] = {SBC, REG_A, REG_C, 1, 1, 1, "SBC"},
    [0x9a] = {SBC, REG_A, REG_D, 1, 1, 1, "SBC"},
    [0x9b] = {SBC, REG_A, REG_E, 1, 1, 1, "SBC"},
    [0x9c] = {SBC, REG_A, REG_H, 1, 1, 1, "SBC"},
    [0x9d] = {SBC, REG_A, REG_L, 1, 1, 1, "SBC"},
    [0x9e] = {SBC, REG_A, PTR_HL, 1, 2, 2, "SBC"},
    [0x9f] = {SBC, REG_A, REG_A, 1, 1, 1, "SBC"},
    [0xa0] = {AND, REG_A, REG_B, 1, 1, 1, "AND"},
    [0xa1] = {AND, REG_A, REG_C, 1, 1, 1, "AND"},
    [0xa2] = {AND, REG_A, REG_D, 1, 1, 1, "AND"},
    [0xa3] = {AND, REG_A, REG_E, 1, 1, 1, "AND"},
    [0xa4] = {AND, REG_A, REG_H, 1, 1, 1, "AND"},
    [0xa5] = {AND, REG_A, REG_L, 1, 1, 1, "AND"},
    [0xa6] = {AND, REG_A, PTR_HL, 1, 2, 2, "AND"},
    [0xa7] = {AND, REG_A, REG_A, 1, 1, 1, "AND"},
    [0xa8] = {XOR, REG_A, REG_B, 1, 1, 1, "XOR"},
    [0xa9] = {XOR, REG_A, REG_C, 1, 1, 1, "XOR"},
    [0xaa] = {XOR, REG_A, REG_D, 1, 1, 1, "XOR"},
    [0xab] = {XOR, REG_A, REG_E, 1, 1, 1, "XOR"},
    [0xac] = {XOR, REG_A, REG_H, 1, 1, 1, "XOR"},
    [0xad] = {XOR, REG_A, REG_L, 1, 1, 1, "XOR"},
    [0xae] = {XOR, REG_A, PTR_HL, 1, 2, 2, "XOR"},
    [0xaf] = {XOR, REG_A, REG_A, 1, 1, 1, "XOR"},
    [0xb0] = {OR, REG_A, REG_B, 1, 1, 1, "OR"},
    [0xb1] = {OR, REG_A, REG_C, 1, 1, 1, "OR"},
    [0xb2] = {OR, REG_A, REG_D, 1, 1, 1, "OR"},
    [0xb3] = {OR, REG_A, REG_E, 1, 1, 1, "OR"},
    [0xb4] = {OR, REG_A, REG_H, 1, 1, 1, "OR"},
    [0xb5] = {OR, REG_A, REG_L, 1, 1, 1, "OR"},
    [0xb6] = {OR, REG_A, PTR_HL, 1, 2, 2, "OR"},
    [0xb7] = {OR, REG_A, REG_A, 1, 1, 1, "OR"},
    [0xb8] = {CP, REG_A, REG_B, 1, 1, 1, "CP"},
    [0xb9] = {CP, REG_A, REG_C, 1, 1, 1, "CP"},
    [0xba] = {CP, REG_A, REG_D, 1, 1, 1, "CP"},
    [0xbb] = {CP, REG_A, REG_E, 1, 1, 1, "CP"},
    [0xbc] = {CP, REG_A, REG_H, 1, 1, 1, "CP"},
    [0xbd] = {CP, REG_A, REG_L, 1, 1, 1, "CP"},
    [0xbe] = {CP, REG_A, PTR_HL, 1, 2, 2, "CP"},
    [0xbf] = {CP, REG_A, REG_A, 1, 1, 1, "CP"},
    [0xc0] = {RET, CC_NZ, NONE, 1, 5, 2, "RET"},
    [0xc1] = {POP, REG_BC, NONE, 1, 3, 3, "POP"},
    [0xc2] = {JP, CC_NZ, IMM_16, 3, 4, 3, "JP"},
    [0xc3] = {JP, IMM_16, NONE, 3, 4, 4, "JP"},
    [0xc4] = {CALL, CC_NZ, IMM_16, 3, 6, 3, "CALL"},
    [0xc5] = {PUSH, REG_BC, NONE, 1, 4, 4, "PUSH"},
    [0xc6] = {ADD, REG_A, IMM_8, 2, 2, 2, "ADD"},
    [0xc7] = {RST, PTR_0x00, NONE, 1, 4, 4, "RST"},
    [0xc8] = {RET, CC_Z, NONE, 1, 5, 2, "RET"},
    [0xc9] = {RET, NONE, NONE, 1, 4, 4, "RET"},
    [0xca] = {JP, CC_Z, IMM_16, 3, 4, 3, "JP"},
    [0xcb] = {PREFIX, NONE, NONE, 1, 1, 1, "PREFIX"}, // 0xCB prefix
    [0xcc] = {CALL, CC_Z, IMM_16, 3, 6, 3, "CALL"},
    [0xcd] = {CALL, IMM_16, NONE, 3, 6, 6, "CALL"},
    [0xce] = {ADC, REG_A, IMM_8, 2, 2, 2, "ADC"},
    [0xcf] = {RST, PTR_0x08, NONE, 1, 4, 4, "RST"},
    [0xd0] = {RET, CC_NC, NONE, 1, 5, 2, "RET"},
    [0xd1] = {POP, REG_DE, NONE, 1, 3, 3, "POP"},
    [0xd2] = {JP, CC_NC, IMM_16, 3, 4, 3, "JP"},
    [0xd3] = {UNUSED, NONE, NONE, 0, 0, 0, "UNUSED"},
    [0xd4] = {CALL, CC_NC, IMM_16, 3, 6, 3, "CALL"},
    [0xd5] = {PUSH, REG_DE, NONE, 1, 4, 4, "PUSH"},
    [0xd6] = {SUB, REG_A, IMM_8, 2, 2, 2, "SUB"},
    [0xd7] = {RST, PTR_0x10, NONE, 1, 4, 4, "RST"},
    [0xd8] = {RET, CC_C, NONE, 1, 5, 2, "RET"},
    [0xd9] = {RETI, NONE, NONE, 1, 4, 4, "RETI"},
    [0xda] = {JP, CC_C, IMM_16, 3, 4, 3, "JP"},
    [0xdb] = {UNUSED, NONE, NONE, 0, 0, 0, "UNUSED"},
    [0xdc] = {CALL, CC_C, IMM_16, 3, 6, 3, "CALL"},
    [0xdd] = {UNUSED, NONE, NONE, 0, 0, 0, "UNUSED"},
    [0xde] = {SBC, REG_A, IMM_8, 2, 2, 2, "SBC"},
    [0xdf] = {RST, PTR_0x18, NONE, 1, 4, 4, "RST"},
    [0xe0] = {LDH, PTR_8, REG_A, 2, 3, 3, "LDH"},
    [0xe1] = {POP, REG_HL, NONE, 1, 3, 3, "POP"},
    [0xe2] = {LDH, PTR_C, REG_A, 1, 2, 2, "LDH"},
    [0xe3] = {UNUSED, NONE, NONE, 0, 0, 0, "UNUSED"},
    [0xe4] = {UNUSED, NONE, NONE, 0, 0, 0, "UNUSED"},
    [0xe5] = {PUSH, REG_HL, NONE, 1, 4, 4, "PUSH"},
    [0xe6] = {AND, REG_A, IMM_8, 2, 2, 2, "AND"},
    [0xe7] = {RST, PTR_0x20, NONE, 1, 4, 4, "RST"},
    [0xe8] = {ADD, REG_SP, IMM_8, 2, 4, 4, "ADD"},
    [0xe9] = {JP, REG_HL, NONE, 1, 1, 1, "JP"},
    [0xea] = {LD, PTR_16, REG_A, 3, 4, 4, "LD"},
    [0xeb] = {UNUSED, NONE, NONE, 0, 0, 0, "UNUSED"},
    [0xec] = {UNUSED, NONE, NONE, 0, 0, 0, "UNUSED"},
    [0xed] = {UNUSED, NONE, NONE, 0, 0, 0, "UNUSED"},
    [0xee] = {XOR, REG_A, IMM_8, 2, 2, 2, "XOR"},
    [0xef] = {RST, PTR_0x28, NONE, 1, 4, 4, "RST"},
    [0xf0] = {LDH, REG_A, PTR_8, 2, 3, 3, "LDH"},
    [0xf1] = {POP, REG_AF, NONE, 1, 3, 3, "POP"},
    [0xf2] = {LDH, REG_A, PTR_C, 1, 2, 2, "LDH"},
    [0xf3] = {DI, NONE, NONE, 1, 1, 1, "DI"},
    [0xf4] = {UNUSED, NONE, NONE, 0, 0, 0, "UNUSED"},
    [0xf5] = {PUSH, REG_AF, NONE, 1, 4, 4, "PUSH"},
    [0xf6] = {OR, REG_A, IMM_8, 2, 2, 2, "OR"},
    [0xf7] = {RST, PTR_0x30, NONE, 1, 4, 4, "RST"},
    [0xf8] = {LD, REG_HL, IMM_8, 2, 3, 3, "LD"},
    [0xf9] = {LD, REG_SP, REG_HL, 1, 2, 2, "LD"},
    [0xfa] = {LD, REG_A, PTR_16, 3, 4, 4, "LD"},
    [0xfb] = {EI, NONE, NONE, 1, 1, 1, "EI"},
    [0xfc] = {UNUSED, NONE, NONE, 0, 0, 0, "UNUSED"},
    [0xfd] = {UNUSED, NONE, NONE, 0, 0, 0, "UNUSED"},
    [0xfe] = {CP, REG_A, IMM_8, 2, 2, 2, "CP"},
    [0xff] = {RST, PTR_0x38, NONE, 1, 4, 4, "RST"},

    // high 256 instructions (0xCB prefixed)
    [0x100] = {RLC, REG_B, NONE, 2, 2, 2, "RLC"},
    [0x101] = {RLC, REG_C, NONE, 2, 2, 2, "RLC"},
    [0x102] = {RLC, REG_D, NONE, 2, 2, 2, "RLC"},
    [0x103] = {RLC, REG_E, NONE, 2, 2, 2, "RLC"},
    [0x104] = {RLC, REG_H, NONE, 2, 2, 2, "RLC"},
    [0x105] = {RLC, REG_L, NONE, 2, 2, 2, "RLC"},
    [0x106] = {RLC, PTR_HL, NONE, 2, 4, 4, "RLC"},
    [0x107] = {RLC, REG_A, NONE, 2, 2, 2, "RLC"},
    [0x108] = {RRC, REG_B, NONE, 2, 2, 2, "RRC"},
    [0x109] = {RRC, REG_C, NONE, 2, 2, 2, "RRC"},
    [0x10a] = {RRC, REG_D, NONE, 2, 2, 2, "RRC"},
    [0x10b] = {RRC, REG_E, NONE, 2, 2, 2, "RRC"},
    [0x10c] = {RRC, REG_H, NONE, 2, 2, 2, "RRC"},
    [0x10d] = {RRC, REG_L, NONE, 2, 2, 2, "RRC"},
    [0x10e] = {RRC, PTR_HL, NONE, 2, 4, 4, "RRC"},
    [0x10f] = {RRC, REG_A, NONE, 2, 2, 2, "RRC"},
    [0x110] = {RL, REG_B, NONE, 2, 2, 2, "RL"},
    [0x111] = {RL, REG_C, NONE, 2, 2, 2, "RL"},
    [0x112] = {RL, REG_D, NONE, 2, 2, 2, "RL"},
    [0x113] = {RL, REG_E, NONE, 2, 2, 2, "RL"},
    [0x114] = {RL, REG_H, NONE, 2, 2, 2, "RL"},
    [0x115] = {RL, REG_L, NONE, 2, 2, 2, "RL"},
    [0x116] = {RL, PTR_HL, NONE, 2, 4, 4, "RL"},
    [0x117] = {RL, REG_A, NONE, 2, 2, 2, "RL"},
    [0x118] = {RR, REG_B, NONE, 2, 2, 2, "RR"},
    [0x119] = {RR, REG_C, NONE, 2, 2, 2, "RR"},
    [0x11a] = {RR, REG_D, NONE, 2, 2, 2, "RR"},
    [0x11b] = {RR, REG_E, NONE, 2, 2, 2, "RR"},
    [0x11c] = {RR, REG_H, NONE, 2, 2, 2, "RR"},
    [0x11d] = {RR, REG_L, NONE, 2, 2, 2, "RR"},
    [0x11e] = {RR, PTR_HL, NONE, 2, 4, 4, "RR"},
    [0x11f] = {RR, REG_A, NONE, 2, 2, 2, "RR"},
    [0x120] = {SLA, REG_B, NONE, 2, 2, 2, "SLA"},
    [0x121] = {SLA, REG_C, NONE, 2, 2, 2, "SLA"},
    [0x122] = {SLA, REG_D, NONE, 2, 2, 2, "SLA"},
    [0x123] = {SLA, REG_E, NONE, 2, 2, 2, "SLA"},
    [0x124] = {SLA, REG_H, NONE, 2, 2, 2, "SLA"},
    [0x125] = {SLA, REG_L, NONE, 2, 2, 2, "SLA"},
    [0x126] = {SLA, PTR_HL, NONE, 2, 4, 4, "SLA"},
    [0x127] = {SLA, REG_A, NONE, 2, 2, 2, "SLA"},
    [0x128] = {SRA, REG_B, NONE, 2, 2, 2, "SRA"},
    [0x129] = {SRA, REG_C, NONE, 2, 2, 2, "SRA"},
    [0x12a] = {SRA, REG_D, NONE, 2, 2, 2, "SRA"},
    [0x12b] = {SRA, REG_E, NONE, 2, 2, 2, "SRA"},
    [0x12c] = {SRA, REG_H, NONE, 2, 2, 2, "SRA"},
    [0x12d] = {SRA, REG_L, NONE, 2, 2, 2, "SRA"},
    [0x12e] = {SRA, PTR_HL, NONE, 2, 4, 4, "SRA"},
    [0x12f] = {SRA, REG_A, NONE, 2, 2, 2, "SRA"},
    [0x130] = {SWAP, REG_B, NONE, 2, 2, 2, "SWAP"},
    [0x131] = {SWAP, REG_C, NONE, 2, 2, 2, "SWAP"},
    [0x132] = {SWAP, REG_D, NONE, 2, 2, 2, "SWAP"},
    [0x133] = {SWAP, REG_E, NONE, 2, 2, 2, "SWAP"},
    [0x134] = {SWAP, REG_H, NONE, 2, 2, 2, "SWAP"},
    [0x135] = {SWAP, REG_L, NONE, 2, 2, 2, "SWAP"},
    [0x136] = {SWAP, PTR_HL, NONE, 2, 4, 4, "SWAP"},
    [0x137] = {SWAP, REG_A, NONE, 2, 2, 2, "SWAP"},
    [0x138] = {SRL, REG_B, NONE, 2, 2, 2, "SRL"},
    [0x139] = {SRL, REG_C, NONE, 2, 2, 2, "SRL"},
    [0x13a] = {SRL, REG_D, NONE, 2, 2, 2, "SRL"},
    [0x13b] = {SRL, REG_E, NONE, 2, 2, 2, "SRL"},
    [0x13c] = {SRL, REG_H, NONE, 2, 2, 2, "SRL"},
    [0x13d] = {SRL, REG_L, NONE, 2, 2, 2, "SRL"},
    [0x13e] = {SRL, PTR_HL, NONE, 2, 4, 4, "SRL"},
    [0x13f] = {SRL, REG_A, NONE, 2, 2, 2, "SRL"},
    [0x140] = {BIT, BIT_0, REG_B, 2, 2, 2, "BIT"},
    [0x141] = {BIT, BIT_0, REG_C, 2, 2, 2, "BIT"},
    [0x142] = {BIT, BIT_0, REG_D, 2, 2, 2, "BIT"},
    [0x143] = {BIT, BIT_0, REG_E, 2, 2, 2, "BIT"},
    [0x144] = {BIT, BIT_0, REG_H, 2, 2, 2, "BIT"},
    [0x145] = {BIT, BIT_0, REG_L, 2, 2, 2, "BIT"},
    [0x146] = {BIT, BIT_0, PTR_HL, 2, 3, 3, "BIT"},
    [0x147] = {BIT, BIT_0, REG_A, 2, 2, 2, "BIT"},
    [0x148] = {BIT, BIT_1, REG_B, 2, 2, 2, "BIT"},
    [0x149] = {BIT, BIT_1, REG_C, 2, 2, 2, "BIT"},
    [0x14a] = {BIT, BIT_1, REG_D, 2, 2, 2, "BIT"},
    [0x14b] = {BIT, BIT_1, REG_E, 2, 2, 2, "BIT"},
    [0x14c] = {BIT, BIT_1, REG_H, 2, 2, 2, "BIT"},
    [0x14d] = {BIT, BIT_1, REG_L, 2, 2, 2, "BIT"},
    [0x14e] = {BIT, BIT_1, PTR_HL, 2, 3, 3, "BIT"},
    [0x14f] = {BIT, BIT_1, REG_A, 2, 2, 2, "BIT"},
    [0x150] = {BIT, BIT_2, REG_B, 2, 2, 2, "BIT"},
    [0x151] = {BIT, BIT_2, REG_C, 2, 2, 2, "BIT"},
    [0x152] = {BIT, BIT_2, REG_D, 2, 2, 2, "BIT"},
    [0x153] = {BIT, BIT_2, REG_E, 2, 2, 2, "BIT"},
    [0x154] = {BIT, BIT_2, REG_H, 2, 2, 2, "BIT"},
    [0x155] = {BIT, BIT_2, REG_L, 2, 2, 2, "BIT"},
    [0x156] = {BIT, BIT_2, PTR_HL, 2, 3, 3, "BIT"},
    [0x157] = {BIT, BIT_2, REG_A, 2, 2, 2, "BIT"},
    [0x158] = {BIT, BIT_3, REG_B, 2, 2, 2, "BIT"},
    [0x159] = {BIT, BIT_3, REG_C, 2, 2, 2, "BIT"},
    [0x15a] = {BIT, BIT_3, REG_D, 2, 2, 2, "BIT"},
    [0x15b] = {BIT, BIT_3, REG_E, 2, 2, 2, "BIT"},
    [0x15c] = {BIT, BIT_3, REG_H, 2, 2, 2, "BIT"},
    [0x15d] = {BIT, BIT_3, REG_L, 2, 2, 2, "BIT"},
    [0x15e] = {BIT, BIT_3, PTR_HL, 2, 3, 3, "BIT"},
    [0x15f] = {BIT, BIT_3, REG_A, 2, 2, 2, "BIT"},
    [0x160] = {BIT, BIT_4, REG_B, 2, 2, 2, "BIT"},
    [0x161] = {BIT, BIT_4, REG_C, 2, 2, 2, "BIT"},
    [0x162] = {BIT, BIT_4, REG_D, 2, 2, 2, "BIT"},
    [0x163] = {BIT, BIT_4, REG_E, 2, 2, 2, "BIT"},
    [0x164] = {BIT, BIT_4, REG_H, 2, 2, 2, "BIT"},
    [0x165] = {BIT, BIT_4, REG_L, 2, 2, 2, "BIT"},
    [0x166] = {BIT, BIT_4, PTR_HL, 2, 3, 3, "BIT"},
    [0x167] = {BIT, BIT_4, REG_A, 2, 2, 2, "BIT"},
    [0x168] = {BIT, BIT_5, REG_B, 2, 2, 2, "BIT"},
    [0x169] = {BIT, BIT_5, REG_C, 2, 2, 2, "BIT"},
    [0x16a] = {BIT, BIT_5, REG_D, 2, 2, 2, "BIT"},
    [0x16b] = {BIT, BIT_5, REG_E, 2, 2, 2, "BIT"},
    [0x16c] = {BIT, BIT_5, REG_H, 2, 2, 2, "BIT"},
    [0x16d] = {BIT, BIT_5, REG_L, 2, 2, 2, "BIT"},
    [0x16e] = {BIT, BIT_5, PTR_HL, 2, 3, 3, "BIT"},
    [0x16f] = {BIT, BIT_5, REG_A, 2, 2, 2, "BIT"},
    [0x170] = {BIT, BIT_6, REG_B, 2, 2, 2, "BIT"},
    [0x171] = {BIT, BIT_6, REG_C, 2, 2, 2, "BIT"},
    [0x172] = {BIT, BIT_6, REG_D, 2, 2, 2, "BIT"},
    [0x173] = {BIT, BIT_6, REG_E, 2, 2, 2, "BIT"},
    [0x174] = {BIT, BIT_6, REG_H, 2, 2, 2, "BIT"},
    [0x175] = {BIT, BIT_6, REG_L, 2, 2, 2, "BIT"},
    [0x176] = {BIT, BIT_6, PTR_HL, 2, 3, 3, "BIT"},
    [0x177] = {BIT, BIT_6, REG_A, 2, 2, 2, "BIT"},
    [0x178] = {BIT, BIT_7, REG_B, 2, 2, 2, "BIT"},
    [0x179] = {BIT, BIT_7, REG_C, 2, 2, 2, "BIT"},
    [0x17a] = {BIT, BIT_7, REG_D, 2, 2, 2, "BIT"},
    [0x17b] = {BIT, BIT_7, REG_E, 2, 2, 2, "BIT"},
    [0x17c] = {BIT, BIT_7, REG_H, 2, 2, 2, "BIT"},
    [0x17d] = {BIT, BIT_7, REG_L, 2, 2, 2, "BIT"},
    [0x17e] = {BIT, BIT_7, PTR_HL, 2, 3, 3, "BIT"},
    [0x17f] = {BIT, BIT_7, REG_A, 2, 2, 2, "BIT"},
    [0x180] = {RES, BIT_0, REG_B, 2, 2, 2, "RES"},
    [0x181] = {RES, BIT_0, REG_C, 2, 2, 2, "RES"},
    [0x182] = {RES, BIT_0, REG_D, 2, 2, 2, "RES"},
    [0x183] = {RES, BIT_0, REG_E, 2, 2, 2, "RES"},
    [0x184] = {RES, BIT_0, REG_H, 2, 2, 2, "RES"},
    [0x185] = {RES, BIT_0, REG_L, 2, 2, 2, "RES"},
    [0x186] = {RES, BIT_0, PTR_HL, 2, 4, 4, "RES"},
    [0x187] = {RES, BIT_0, REG_A, 2, 2, 2, "RES"},
    [0x188] = {RES, BIT_1, REG_B, 2, 2, 2, "RES"},
    [0x189] = {RES, BIT_1, REG_C, 2, 2, 2, "RES"},
    [0x18a] = {RES, BIT_1, REG_D, 2, 2, 2, "RES"},
    [0x18b] = {RES, BIT_1, REG_E, 2, 2, 2, "RES"},
    [0x18c] = {RES, BIT_1, REG_H, 2, 2, 2, "RES"},
    [0x18d] = {RES, BIT_1, REG_L, 2, 2, 2, "RES"},
    [0x18e] = {RES, BIT_1, PTR_HL, 2, 4, 4, "RES"},
    [0x18f] = {RES, BIT_1, REG_A, 2, 2, 2, "RES"},
    [0x190] = {RES, BIT_2, REG_B, 2, 2, 2, "RES"},
    [0x191] = {RES, BIT_2, REG_C, 2, 2, 2, "RES"},
    [0x192] = {RES, BIT_2, REG_D, 2, 2, 2, "RES"},
    [0x193] = {RES, BIT_2, REG_E, 2, 2, 2, "RES"},
    [0x194] = {RES, BIT_2, REG_H, 2, 2, 2, "RES"},
    [0x195] = {RES, BIT_2, REG_L, 2, 2, 2, "RES"},
    [0x196] = {RES, BIT_2, PTR_HL, 2, 4, 4, "RES"},
    [0x197] = {RES, BIT_2, REG_A, 2, 2, 2, "RES"},
    [0x198] = {RES, BIT_3, REG_B, 2, 2, 2, "RES"},
    [0x199] = {RES, BIT_3, REG_C, 2, 2, 2, "RES"},
    [0x19a] = {RES, BIT_3, REG_D, 2, 2, 2, "RES"},
    [0x19b] = {RES, BIT_3, REG_E, 2, 2, 2, "RES"},
    [0x19c] = {RES, BIT_3, REG_H, 2, 2, 2, "RES"},
    [0x19d] = {RES, BIT_3, REG_L, 2, 2, 2, "RES"},
    [0x19e] = {RES, BIT_3, PTR_HL, 2, 4, 4, "RES"},
    [0x19f] = {RES, BIT_3, REG_A, 2, 2, 2, "RES"},
    [0x1a0] = {RES, BIT_4, REG_B, 2, 2, 2, "RES"},
    [0x1a1] = {RES, BIT_4, REG_C, 2, 2, 2, "RES"},
    [0x1a2] = {RES, BIT_4, REG_D, 2, 2, 2, "RES"},
    [0x1a3] = {RES, BIT_4, REG_E, 2, 2, 2, "RES"},
    [0x1a4] = {RES, BIT_4, REG_H, 2, 2, 2, "RES"},
    [0x1a5] = {RES, BIT_4, REG_L, 2, 2, 2, "RES"},
    [0x1a6] = {RES, BIT_4, PTR_HL, 2, 4, 4, "RES"},
    [0x1a7] = {RES, BIT_4, REG_A, 2, 2, 2, "RES"},
    [0x1a8] = {RES, BIT_5, REG_B, 2, 2, 2, "RES"},
    [0x1a9] = {RES, BIT_5, REG_C, 2, 2, 2, "RES"},
    [0x1aa] = {RES, BIT_5, REG_D, 2, 2, 2, "RES"},
    [0x1ab] = {RES, BIT_5, REG_E, 2, 2, 2, "RES"},
    [0x1ac] = {RES, BIT_5, REG_H, 2, 2, 2, "RES"},
    [0x1ad] = {RES, BIT_5, REG_L, 2, 2, 2, "RES"},
    [0x1ae] = {RES, BIT_5, PTR_HL, 2, 4, 4, "RES"},
    [0x1af] = {RES, BIT_5, REG_A, 2, 2, 2, "RES"},
    [0x1b0] = {RES, BIT_6, REG_B, 2, 2, 2, "RES"},
    [0x1b1] = {RES, BIT_6, REG_C, 2, 2, 2, "RES"},
    [0x1b2] = {RES, BIT_6, REG_D, 2, 2, 2, "RES"},
    [0x1b3] = {RES, BIT_6, REG_E, 2, 2, 2, "RES"},
    [0x1b4] = {RES, BIT_6, REG_H, 2, 2, 2, "RES"},
    [0x1b5] = {RES, BIT_6, REG_L, 2, 2, 2, "RES"},
    [0x1b6] = {RES, BIT_6, PTR_HL, 2, 4, 4, "RES"},
    [0x1b7] = {RES, BIT_6, REG_A, 2, 2, 2, "RES"},
    [0x1b8] = {RES, BIT_7, REG_B, 2, 2, 2, "RES"},
    [0x1b9] = {RES, BIT_7, REG_C, 2, 2, 2, "RES"},
    [0x1ba] = {RES, BIT_7, REG_D, 2, 2, 2, "RES"},
    [0x1bb] = {RES, BIT_7, REG_E, 2, 2, 2, "RES"},
    [0x1bc] = {RES, BIT_7, REG_H, 2, 2, 2, "RES"},
    [0x1bd] = {RES, BIT_7, REG_L, 2, 2, 2, "RES"},
    [0x1be] = {RES, BIT_7, PTR_HL, 2, 4, 4, "RES"},
    [0x1bf] = {RES, BIT_7, REG_A, 2, 2, 2, "RES"},
    [0x1c0] = {SET, BIT_0, REG_B, 2, 2, 2, "SET"},
    [0x1c1] = {SET, BIT_0, REG_C, 2, 2, 2, "SET"},
    [0x1c2] = {SET, BIT_0, REG_D, 2, 2, 2, "SET"},
    [0x1c3] = {SET, BIT_0, REG_E, 2, 2, 2, "SET"},
    [0x1c4] = {SET, BIT_0, REG_H, 2, 2, 2, "SET"},
    [0x1c5] = {SET, BIT_0, REG_L, 2, 2, 2, "SET"},
    [0x1c6] = {SET, BIT_0, PTR_HL, 2, 4, 4, "SET"},
    [0x1c7] = {SET, BIT_0, REG_A, 2, 2, 2, "SET"},
    [0x1c8] = {SET, BIT_1, REG_B, 2, 2, 2, "SET"},
    [0x1c9] = {SET, BIT_1, REG_C, 2, 2, 2, "SET"},
    [0x1ca] = {SET, BIT_1, REG_D, 2, 2, 2, "SET"},
    [0x1cb] = {SET, BIT_1, REG_E, 2, 2, 2, "SET"},
    [0x1cc] = {SET, BIT_1, REG_H, 2, 2, 2, "SET"},
    [0x1cd] = {SET, BIT_1, REG_L, 2, 2, 2, "SET"},
    [0x1ce] = {SET, BIT_1, PTR_HL, 2, 4, 4, "SET"},
    [0x1cf] = {SET, BIT_1, REG_A, 2, 2, 2, "SET"},
    [0x1d0] = {SET, BIT_2, REG_B, 2, 2, 2, "SET"},
    [0x1d1] = {SET, BIT_2, REG_C, 2, 2, 2, "SET"},
    [0x1d2] = {SET, BIT_2, REG_D, 2, 2, 2, "SET"},
    [0x1d3] = {SET, BIT_2, REG_E, 2, 2, 2, "SET"},
    [0x1d4] = {SET, BIT_2, REG_H, 2, 2, 2, "SET"},
    [0x1d5] = {SET, BIT_2, REG_L, 2, 2, 2, "SET"},
    [0x1d6] = {SET, BIT_2, PTR_HL, 2, 4, 4, "SET"},
    [0x1d7] = {SET, BIT_2, REG_A, 2, 2, 2, "SET"},
    [0x1d8] = {SET, BIT_3, REG_B, 2, 2, 2, "SET"},
    [0x1d9] = {SET, BIT_3, REG_C, 2, 2, 2, "SET"},
    [0x1da] = {SET, BIT_3, REG_D, 2, 2, 2, "SET"},
    [0x1db] = {SET, BIT_3, REG_E, 2, 2, 2, "SET"},
    [0x1dc] = {SET, BIT_3, REG_H, 2, 2, 2, "SET"},
    [0x1dd] = {SET, BIT_3, REG_L, 2, 2, 2, "SET"},
    [0x1de] = {SET, BIT_3, PTR_HL, 2, 4, 4, "SET"},
    [0x1df] = {SET, BIT_3, REG_A, 2, 2, 2, "SET"},
    [0x1e0] = {SET, BIT_4, REG_B, 2, 2, 2, "SET"},
    [0x1e1] = {SET, BIT_4, REG_C, 2, 2, 2, "SET"},
    [0x1e2] = {SET, BIT_4, REG_D, 2, 2, 2, "SET"},
    [0x1e3] = {SET, BIT_4, REG_E, 2, 2, 2, "SET"},
    [0x1e4] = {SET, BIT_4, REG_H, 2, 2, 2, "SET"},
    [0x1e5] = {SET, BIT_4, REG_L, 2, 2, 2, "SET"},
    [0x1e6] = {SET, BIT_4, PTR_HL, 2, 4, 4, "SET"},
    [0x1e7] = {SET, BIT_4, REG_A, 2, 2, 2, "SET"},
    [0x1e8] = {SET, BIT_5, REG_B, 2, 2, 2, "SET"},
    [0x1e9] = {SET, BIT_5, REG_C, 2, 2, 2, "SET"},
    [0x1ea] = {SET, BIT_5, REG_D, 2, 2, 2, "SET"},
    [0x1eb] = {SET, BIT_5, REG_E, 2, 2, 2, "SET"},
    [0x1ec] = {SET, BIT_5, REG_H, 2, 2, 2, "SET"},
    [0x1ed] = {SET, BIT_5, REG_L, 2, 2, 2, "SET"},
    [0x1ee] = {SET, BIT_5, PTR_HL, 2, 4, 4, "SET"},
    [0x1ef] = {SET, BIT_5, REG_A, 2, 2, 2, "SET"},
    [0x1f0] = {SET, BIT_6, REG_B, 2, 2, 2, "SET"},
    [0x1f1] = {SET, BIT_6, REG_C, 2, 2, 2, "SET"},
    [0x1f2] = {SET, BIT_6, REG_D, 2, 2, 2, "SET"},
    [0x1f3] = {SET, BIT_6, REG_E, 2, 2, 2, "SET"},
    [0x1f4] = {SET, BIT_6, REG_H, 2, 2, 2, "SET"},
    [0x1f5] = {SET, BIT_6, REG_L, 2, 2, 2, "SET"},
    [0x1f6] = {SET, BIT_6, PTR_HL, 2, 4, 4, "SET"},
    [0x1f7] = {SET, BIT_6, REG_A, 2, 2, 2, "SET"},
    [0x1f8] = {SET, BIT_7, REG_B, 2, 2, 2, "SET"},
    [0x1f9] = {SET, BIT_7, REG_C, 2, 2, 2, "SET"},
    [0x1fa] = {SET, BIT_7, REG_D, 2, 2, 2, "SET"},
    [0x1fb] = {SET, BIT_7, REG_E, 2, 2, 2, "SET"},
    [0x1fc] = {SET, BIT_7, REG_H, 2, 2, 2, "SET"},
    [0x1fd] = {SET, BIT_7, REG_L, 2, 2, 2, "SET"},
    [0x1fe] = {SET, BIT_7, PTR_HL, 2, 4, 4, "SET"},
    [0x1ff] = {SET, BIT_7, REG_A, 2, 2, 2, "SET"},
};

// returns the number of m-cycles elapsed during instruction execution
uint8_t execute_instruction(gameboy *gb)
{
    // the instruction's duration
    uint8_t curr_inst_duration;

    // if an interrupt is pending, service it
    // instead of executing the next instruction
    curr_inst_duration = service_interrupt(gb);
    if (curr_inst_duration)
        goto interrupt_serviced;

    uint8_t inst_code;
    if (!gb->cpu->halt_bug)
    {
        inst_code = read_byte(gb, (gb->cpu->reg->pc)++);
    }
    else // HALT bug. PC fails to be incremented once
    {
        inst_code = read_byte(gb, gb->cpu->reg->pc);
        gb->cpu->halt_bug = false;
    }

    gb_instruction inst = instruction_table[inst_code];

    /* Check if we need to access a prefixed instruction.
     *
     * NOTE: Currently, all of the prefixed instructions
     * account for the time needed to read the prefix byte
     * in their duration. They also account for the prefix
     * byte in their instruction length (though the length
     * is not currently used explicitly for any instruction).
     *
     * TODO: maybe account for prefix duration separately.
     */
    if (inst.opcode == PREFIX)
    {
        // read the prefixed instruction code and access instruction
        inst_code = read_byte(gb, (gb->cpu->reg->pc)++);
        inst = instruction_table[0x100 + inst_code];
    }

    switch (inst.opcode)
    {
        case NOP:
            curr_inst_duration = inst.duration;
            LOG_DEBUG("NOP\n");
            break;

        case LD:
            ld(gb, &inst);
            curr_inst_duration = inst.duration;
            break;

        case LDH:
            ldh(gb, &inst);
            curr_inst_duration = inst.duration;
            break;

        case INC:
            inc(gb, &inst);
            curr_inst_duration = inst.duration;
            break;

        case DEC:
            dec(gb, &inst);
            curr_inst_duration = inst.duration;
            break;

        case ADD:
            add(gb, &inst);
            curr_inst_duration = inst.duration;
            break;

        case ADC:
            adc(gb, &inst);
            curr_inst_duration = inst.duration;
            break;

        case SUB:
            sub(gb, &inst);
            curr_inst_duration = inst.duration;
            break;

        case SBC:
            sbc(gb, &inst);
            curr_inst_duration = inst.duration;
            break;

        case CP:
            cp(gb, &inst);
            curr_inst_duration = inst.duration;
            break;

        case AND:
            and(gb, &inst);
            curr_inst_duration = inst.duration;
            break;

        case OR:
            or(gb, &inst);
            curr_inst_duration = inst.duration;
            break;

        case XOR:
            xor(gb, &inst);
            curr_inst_duration = inst.duration;
            break;

        case JP:
            curr_inst_duration = jp(gb, &inst);
            break;

        case JR:
            curr_inst_duration = jr(gb, &inst);
            break;

        case CALL:
            curr_inst_duration = call(gb, &inst);
            break;

        case RST:
            rst(gb, &inst);
            curr_inst_duration = inst.duration;
            break;

        case RET:
            curr_inst_duration = ret(gb, &inst);
            break;

        case EI:
            ei(gb);
            curr_inst_duration = inst.duration;
            break;

        case DI:
            di(gb);
            curr_inst_duration = inst.duration;
            break;

        case RETI:
            reti(gb);
            curr_inst_duration = inst.duration;
            break;

        case POP:
            pop(gb, &inst);
            curr_inst_duration = inst.duration;
            break;

        case PUSH:
            push(gb, &inst);
            curr_inst_duration = inst.duration;
            break;

        case RLCA:
            rlca(gb);
            curr_inst_duration = inst.duration;
            break;

        case RRCA:
            rrca(gb);
            curr_inst_duration = inst.duration;
            break;

        case RLA:
            rla(gb);
            curr_inst_duration = inst.duration;
            break;

        case RRA:
            rra(gb);
            curr_inst_duration = inst.duration;
            break;

        case DAA:
            daa(gb);
            curr_inst_duration = inst.duration;
            break;

        case SCF:
            scf(gb);
            curr_inst_duration = inst.duration;
            break;

        case CPL:
            cpl(gb);
            curr_inst_duration = inst.duration;
            break;

        case CCF:
            ccf(gb);
            curr_inst_duration = inst.duration;
            break;

        case STOP:
            stop(gb);
            curr_inst_duration = inst.duration;
            break;

        case HALT:
            halt(gb);
            curr_inst_duration = inst.duration;
            break;

        // CB Opcodes
        case RLC:
            rlc(gb, &inst);
            curr_inst_duration = inst.duration;
            break;

        case RRC:
            rrc(gb, &inst);
            curr_inst_duration = inst.duration;
            break;

        case RL:
            rl(gb, &inst);
            curr_inst_duration = inst.duration;
            break;

        case RR:
            rr(gb, &inst);
            curr_inst_duration = inst.duration;
            break;

        case SLA:
            sla(gb, &inst);
            curr_inst_duration = inst.duration;
            break;

        case SRA:
            sra(gb, &inst);
            curr_inst_duration = inst.duration;
            break;

        case SWAP:
            swap(gb, &inst);
            curr_inst_duration = inst.duration;
            break;

        case SRL:
            srl(gb, &inst);
            curr_inst_duration = inst.duration;
            break;

        case BIT:
            bit(gb, &inst);
            curr_inst_duration = inst.duration;
            break;

        case RES:
            res(gb, &inst);
            curr_inst_duration = inst.duration;
            break;

        case SET:
            set(gb, &inst);
            curr_inst_duration = inst.duration;
            break;

        /* using UNUSED opcodes isn't necessarily wrong,
         * but we may want to know this happened when
         * we're debugging because it seems unlikely
         * these would be used on purpose
         */
        case UNUSED:
            LOG_DEBUG("Opcode UNUSED was encountered. This may be a bug.\n");
            curr_inst_duration = inst.duration;
            break;

        // panic if we encounter an illegal opcode
        default:
            LOG_ERROR("Illegal opcode (0x%02X) was encountered. Exiting...\n", inst_code);
            exit(1);
    }

interrupt_serviced:
    /* Check if the IME flag needs to be set after
     * an EI instruction. The IME is set after the
     * instruction following the EI.
     */
    if (gb->cpu->ime_delayed_set)
    {
        gb->cpu->ime_flag = true;
        gb->cpu->ime_delayed_set = false;
    }

    return curr_inst_duration;
}
