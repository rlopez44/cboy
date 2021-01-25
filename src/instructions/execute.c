#include <stdint.h>
#include "cboy/instructions.h"
#include "cboy/cpu.h"
#include "cboy/memory.h"
#include "execute.h"

// the CPU's instruction set
static const gb_instruction instruction_table[512] = {
    /* FORMAT (durations specified in M-cycles)
     * ------
     * [NUMERIC_CODE] = {OPCODE, OP1, OP2, LENGTH, DURATION, ALT_DURATION},
     */

    // low 256 instructions
    [0x00] = {NOP, NONE, NONE, 1, 1, 1},
    [0x01] = {LD, REG_BC, IMM_16, 3, 3, 3},
    [0x02] = {LD, PTR_BC, REG_A, 1, 2, 2},
    [0x03] = {INC, REG_BC, NONE, 1, 2, 2},
    [0x04] = {INC, REG_B, NONE, 1, 1, 1},
    [0x05] = {DEC, REG_B, NONE, 1, 1, 1},
    [0x06] = {LD, REG_B, IMM_8, 2, 2, 2},
    [0x07] = {RLCA, REG_A, NONE, 1, 1, 1},
    [0x08] = {LD, PTR_16, REG_SP, 3, 5, 5},
    [0x09] = {ADD, REG_HL, REG_BC, 1, 2, 2},
    [0x0a] = {LD, REG_A, PTR_BC, 1, 2, 2},
    [0x0b] = {DEC, REG_BC, NONE, 1, 2, 2},
    [0x0c] = {INC, REG_C, NONE, 1, 1, 1},
    [0x0d] = {DEC, REG_C, NONE, 1, 1, 1},
    [0x0e] = {LD, REG_C, IMM_8, 2, 2, 2},
    [0x0f] = {RRCA, REG_A, NONE, 1, 1, 1},
    [0x10] = {STOP, NONE, NONE, 2, 1, 1},
    [0x11] = {LD, REG_DE, IMM_16, 3, 3, 3},
    [0x12] = {LD, PTR_DE, REG_A, 1, 2, 2},
    [0x13] = {INC, REG_DE, NONE, 1, 2, 2},
    [0x14] = {INC, REG_D, NONE, 1, 1, 1},
    [0x15] = {DEC, REG_D, NONE, 1, 1, 1},
    [0x16] = {LD, REG_D, IMM_8, 2, 2, 2},
    [0x17] = {RLA, REG_A, NONE, 1, 1, 1},
    [0x18] = {JR, IMM_8, NONE, 2, 3, 3},
    [0x19] = {ADD, REG_HL, REG_DE, 1, 2, 2},
    [0x1a] = {LD, REG_A, PTR_DE, 1, 2, 2},
    [0x1b] = {DEC, REG_DE, NONE, 1, 2, 2},
    [0x1c] = {INC, REG_E, NONE, 1, 1, 1},
    [0x1d] = {DEC, REG_E, NONE, 1, 1, 1},
    [0x1e] = {LD, REG_E, IMM_8, 2, 2, 2},
    [0x1f] = {RRA, REG_A, NONE, 1, 1, 1},
    [0x20] = {JR, CC_NZ, IMM_8, 2, 3, 2},
    [0x21] = {LD, REG_HL, IMM_16, 3, 3, 3},
    [0x22] = {LD, PTR_HL_INC, REG_A, 1, 2, 2},
    [0x23] = {INC, REG_HL, NONE, 1, 2, 2},
    [0x24] = {INC, REG_H, NONE, 1, 1, 1},
    [0x25] = {DEC, REG_H, NONE, 1, 1, 1},
    [0x26] = {LD, REG_H, IMM_8, 2, 2, 2},
    [0x27] = {DAA, NONE, NONE, 1, 1, 1},
    [0x28] = {JR, CC_Z, IMM_8, 2, 3, 2},
    [0x29] = {ADD, REG_HL, REG_HL, 1, 2, 2},
    [0x2a] = {LD, REG_A, PTR_HL_INC, 1, 2, 2},
    [0x2b] = {DEC, REG_HL, NONE, 1, 2, 2},
    [0x2c] = {INC, REG_L, NONE, 1, 1, 1},
    [0x2d] = {DEC, REG_L, NONE, 1, 1, 1},
    [0x2e] = {LD, REG_L, IMM_8, 2, 2, 2},
    [0x2f] = {CPL, REG_A, NONE, 1, 1, 1},
    [0x30] = {JR, CC_NC, IMM_8, 2, 3, 2},
    [0x31] = {LD, REG_SP, IMM_16, 3, 3, 3},
    [0x32] = {LD, PTR_HL_DEC, REG_A, 1, 2, 2},
    [0x33] = {INC, REG_SP, NONE, 1, 2, 2},
    [0x34] = {INC, PTR_HL, NONE, 1, 3, 3},
    [0x35] = {DEC, PTR_HL, NONE, 1, 3, 3},
    [0x36] = {LD, PTR_HL, IMM_8, 2, 3, 3},
    [0x37] = {SCF, NONE, NONE, 1, 1, 1},
    [0x38] = {JR, CC_C, IMM_8, 2, 3, 2},
    [0x39] = {ADD, REG_HL, REG_SP, 1, 2, 2},
    [0x3a] = {LD, REG_A, PTR_HL_DEC, 1, 2, 2},
    [0x3b] = {DEC, REG_SP, NONE, 1, 2, 2},
    [0x3c] = {INC, REG_A, NONE, 1, 1, 1},
    [0x3d] = {DEC, REG_A, NONE, 1, 1, 1},
    [0x3e] = {LD, REG_A, IMM_8, 2, 2, 2},
    [0x3f] = {CCF, NONE, NONE, 1, 1, 1},
    [0x40] = {LD, REG_B, REG_B, 1, 1, 1},
    [0x41] = {LD, REG_B, REG_C, 1, 1, 1},
    [0x42] = {LD, REG_B, REG_D, 1, 1, 1},
    [0x43] = {LD, REG_B, REG_E, 1, 1, 1},
    [0x44] = {LD, REG_B, REG_H, 1, 1, 1},
    [0x45] = {LD, REG_B, REG_L, 1, 1, 1},
    [0x46] = {LD, REG_B, PTR_HL, 1, 2, 2},
    [0x47] = {LD, REG_B, REG_A, 1, 1, 1},
    [0x48] = {LD, REG_C, REG_B, 1, 1, 1},
    [0x49] = {LD, REG_C, REG_C, 1, 1, 1},
    [0x4a] = {LD, REG_C, REG_D, 1, 1, 1},
    [0x4b] = {LD, REG_C, REG_E, 1, 1, 1},
    [0x4c] = {LD, REG_C, REG_H, 1, 1, 1},
    [0x4d] = {LD, REG_C, REG_L, 1, 1, 1},
    [0x4e] = {LD, REG_C, PTR_HL, 1, 2, 2},
    [0x4f] = {LD, REG_C, REG_A, 1, 1, 1},
    [0x50] = {LD, REG_D, REG_B, 1, 1, 1},
    [0x51] = {LD, REG_D, REG_C, 1, 1, 1},
    [0x52] = {LD, REG_D, REG_D, 1, 1, 1},
    [0x53] = {LD, REG_D, REG_E, 1, 1, 1},
    [0x54] = {LD, REG_D, REG_H, 1, 1, 1},
    [0x55] = {LD, REG_D, REG_L, 1, 1, 1},
    [0x56] = {LD, REG_D, PTR_HL, 1, 2, 2},
    [0x57] = {LD, REG_D, REG_A, 1, 1, 1},
    [0x58] = {LD, REG_E, REG_B, 1, 1, 1},
    [0x59] = {LD, REG_E, REG_C, 1, 1, 1},
    [0x5a] = {LD, REG_E, REG_D, 1, 1, 1},
    [0x5b] = {LD, REG_E, REG_E, 1, 1, 1},
    [0x5c] = {LD, REG_E, REG_H, 1, 1, 1},
    [0x5d] = {LD, REG_E, REG_L, 1, 1, 1},
    [0x5e] = {LD, REG_E, PTR_HL, 1, 2, 2},
    [0x5f] = {LD, REG_E, REG_A, 1, 1, 1},
    [0x60] = {LD, REG_H, REG_B, 1, 1, 1},
    [0x61] = {LD, REG_H, REG_C, 1, 1, 1},
    [0x62] = {LD, REG_H, REG_D, 1, 1, 1},
    [0x63] = {LD, REG_H, REG_E, 1, 1, 1},
    [0x64] = {LD, REG_H, REG_H, 1, 1, 1},
    [0x65] = {LD, REG_H, REG_L, 1, 1, 1},
    [0x66] = {LD, REG_H, PTR_HL, 1, 2, 2},
    [0x67] = {LD, REG_H, REG_A, 1, 1, 1},
    [0x68] = {LD, REG_L, REG_B, 1, 1, 1},
    [0x69] = {LD, REG_L, REG_C, 1, 1, 1},
    [0x6a] = {LD, REG_L, REG_D, 1, 1, 1},
    [0x6b] = {LD, REG_L, REG_E, 1, 1, 1},
    [0x6c] = {LD, REG_L, REG_H, 1, 1, 1},
    [0x6d] = {LD, REG_L, REG_L, 1, 1, 1},
    [0x6e] = {LD, REG_L, PTR_HL, 1, 2, 2},
    [0x6f] = {LD, REG_L, REG_A, 1, 1, 1},
    [0x70] = {LD, PTR_HL, REG_B, 1, 2, 2},
    [0x71] = {LD, PTR_HL, REG_C, 1, 2, 2},
    [0x72] = {LD, PTR_HL, REG_D, 1, 2, 2},
    [0x73] = {LD, PTR_HL, REG_E, 1, 2, 2},
    [0x74] = {LD, PTR_HL, REG_H, 1, 2, 2},
    [0x75] = {LD, PTR_HL, REG_L, 1, 2, 2},
    [0x76] = {HALT, NONE, NONE, 1, 1, 1},
    [0x77] = {LD, PTR_HL, REG_A, 1, 2, 2},
    [0x78] = {LD, REG_A, REG_B, 1, 1, 1},
    [0x79] = {LD, REG_A, REG_C, 1, 1, 1},
    [0x7a] = {LD, REG_A, REG_D, 1, 1, 1},
    [0x7b] = {LD, REG_A, REG_E, 1, 1, 1},
    [0x7c] = {LD, REG_A, REG_H, 1, 1, 1},
    [0x7d] = {LD, REG_A, REG_L, 1, 1, 1},
    [0x7e] = {LD, REG_A, PTR_HL, 1, 2, 2},
    [0x7f] = {LD, REG_A, REG_A, 1, 1, 1},
    [0x80] = {ADD, REG_A, REG_B, 1, 1, 1},
    [0x81] = {ADD, REG_A, REG_C, 1, 1, 1},
    [0x82] = {ADD, REG_A, REG_D, 1, 1, 1},
    [0x83] = {ADD, REG_A, REG_E, 1, 1, 1},
    [0x84] = {ADD, REG_A, REG_H, 1, 1, 1},
    [0x85] = {ADD, REG_A, REG_L, 1, 1, 1},
    [0x86] = {ADD, REG_A, PTR_HL, 1, 2, 2},
    [0x87] = {ADD, REG_A, REG_A, 1, 1, 1},
    [0x88] = {ADC, REG_A, REG_B, 1, 1, 1},
    [0x89] = {ADC, REG_A, REG_C, 1, 1, 1},
    [0x8a] = {ADC, REG_A, REG_D, 1, 1, 1},
    [0x8b] = {ADC, REG_A, REG_E, 1, 1, 1},
    [0x8c] = {ADC, REG_A, REG_H, 1, 1, 1},
    [0x8d] = {ADC, REG_A, REG_L, 1, 1, 1},
    [0x8e] = {ADC, REG_A, PTR_HL, 1, 2, 2},
    [0x8f] = {ADC, REG_A, REG_A, 1, 1, 1},
    [0x90] = {SUB, REG_A, REG_B, 1, 1, 1},
    [0x91] = {SUB, REG_A, REG_C, 1, 1, 1},
    [0x92] = {SUB, REG_A, REG_D, 1, 1, 1},
    [0x93] = {SUB, REG_A, REG_E, 1, 1, 1},
    [0x94] = {SUB, REG_A, REG_H, 1, 1, 1},
    [0x95] = {SUB, REG_A, REG_L, 1, 1, 1},
    [0x96] = {SUB, REG_A, PTR_HL, 1, 2, 2},
    [0x97] = {SUB, REG_A, REG_A, 1, 1, 1},
    [0x98] = {SBC, REG_A, REG_B, 1, 1, 1},
    [0x99] = {SBC, REG_A, REG_C, 1, 1, 1},
    [0x9a] = {SBC, REG_A, REG_D, 1, 1, 1},
    [0x9b] = {SBC, REG_A, REG_E, 1, 1, 1},
    [0x9c] = {SBC, REG_A, REG_H, 1, 1, 1},
    [0x9d] = {SBC, REG_A, REG_L, 1, 1, 1},
    [0x9e] = {SBC, REG_A, PTR_HL, 1, 2, 2},
    [0x9f] = {SBC, REG_A, REG_A, 1, 1, 1},
    [0xa0] = {AND, REG_A, REG_B, 1, 1, 1},
    [0xa1] = {AND, REG_A, REG_C, 1, 1, 1},
    [0xa2] = {AND, REG_A, REG_D, 1, 1, 1},
    [0xa3] = {AND, REG_A, REG_E, 1, 1, 1},
    [0xa4] = {AND, REG_A, REG_H, 1, 1, 1},
    [0xa5] = {AND, REG_A, REG_L, 1, 1, 1},
    [0xa6] = {AND, REG_A, PTR_HL, 1, 2, 2},
    [0xa7] = {AND, REG_A, REG_A, 1, 1, 1},
    [0xa8] = {XOR, REG_A, REG_B, 1, 1, 1},
    [0xa9] = {XOR, REG_A, REG_C, 1, 1, 1},
    [0xaa] = {XOR, REG_A, REG_D, 1, 1, 1},
    [0xab] = {XOR, REG_A, REG_E, 1, 1, 1},
    [0xac] = {XOR, REG_A, REG_H, 1, 1, 1},
    [0xad] = {XOR, REG_A, REG_L, 1, 1, 1},
    [0xae] = {XOR, REG_A, PTR_HL, 1, 2, 2},
    [0xaf] = {XOR, REG_A, REG_A, 1, 1, 1},
    [0xb0] = {OR, REG_A, REG_B, 1, 1, 1},
    [0xb1] = {OR, REG_A, REG_C, 1, 1, 1},
    [0xb2] = {OR, REG_A, REG_D, 1, 1, 1},
    [0xb3] = {OR, REG_A, REG_E, 1, 1, 1},
    [0xb4] = {OR, REG_A, REG_H, 1, 1, 1},
    [0xb5] = {OR, REG_A, REG_L, 1, 1, 1},
    [0xb6] = {OR, REG_A, PTR_HL, 1, 2, 2},
    [0xb7] = {OR, REG_A, REG_A, 1, 1, 1},
    [0xb8] = {CP, REG_A, REG_B, 1, 1, 1},
    [0xb9] = {CP, REG_A, REG_C, 1, 1, 1},
    [0xba] = {CP, REG_A, REG_D, 1, 1, 1},
    [0xbb] = {CP, REG_A, REG_E, 1, 1, 1},
    [0xbc] = {CP, REG_A, REG_H, 1, 1, 1},
    [0xbd] = {CP, REG_A, REG_L, 1, 1, 1},
    [0xbe] = {CP, REG_A, PTR_HL, 1, 2, 2},
    [0xbf] = {CP, REG_A, REG_A, 1, 1, 1},
    [0xc0] = {RET, CC_NZ, NONE, 1, 5, 2},
    [0xc1] = {POP, REG_BC, NONE, 1, 3, 3},
    [0xc2] = {JP, CC_NZ, IMM_16, 3, 4, 3},
    [0xc3] = {JP, IMM_16, NONE, 3, 4, 4},
    [0xc4] = {CALL, CC_NZ, IMM_16, 3, 6, 3},
    [0xc5] = {PUSH, REG_BC, NONE, 1, 4, 4},
    [0xc6] = {ADD, REG_A, IMM_8, 2, 2, 2},
    [0xc7] = {RST, PTR_0x00, NONE, 1, 4, 4},
    [0xc8] = {RET, CC_Z, NONE, 1, 5, 2},
    [0xc9] = {RET, NONE, NONE, 1, 4, 4},
    [0xca] = {JP, CC_Z, IMM_16, 3, 4, 3},
    [0xcb] = {PREFIX, NONE, NONE, 1, 1, 1}, // 0xCB prefix
    [0xcc] = {CALL, CC_Z, IMM_16, 3, 6, 3},
    [0xcd] = {CALL, IMM_16, NONE, 3, 6, 6},
    [0xce] = {ADC, REG_A, IMM_8, 2, 2, 2},
    [0xcf] = {RST, PTR_0x08, NONE, 1, 4, 4},
    [0xd0] = {RET, CC_NC, NONE, 1, 5, 2},
    [0xd1] = {POP, REG_DE, NONE, 1, 3, 3},
    [0xd2] = {JP, CC_NC, IMM_16, 3, 4, 3},
    [0xd3] = {UNUSED, NONE, NONE, 0, 0, 0},
    [0xd4] = {CALL, CC_NC, IMM_16, 3, 6, 3},
    [0xd5] = {PUSH, REG_DE, NONE, 1, 4, 4},
    [0xd6] = {SUB, REG_A, IMM_8, 2, 2, 2},
    [0xd7] = {RST, PTR_0x10, NONE, 1, 4, 4},
    [0xd8] = {RET, CC_C, NONE, 1, 5, 2},
    [0xd9] = {RETI, NONE, NONE, 1, 4, 4},
    [0xda] = {JP, CC_C, IMM_16, 3, 4, 3},
    [0xdb] = {UNUSED, NONE, NONE, 0, 0, 0},
    [0xdc] = {CALL, CC_C, IMM_16, 3, 6, 3},
    [0xdd] = {UNUSED, NONE, NONE, 0, 0, 0},
    [0xde] = {SBC, REG_A, IMM_8, 2, 2, 2},
    [0xdf] = {RST, PTR_0x18, NONE, 1, 4, 4},
    [0xe0] = {LDH, PTR_8, REG_A, 2, 3, 3},
    [0xe1] = {POP, REG_HL, NONE, 1, 3, 3},
    [0xe2] = {LDH, PTR_C, REG_A, 1, 2, 2},
    [0xe3] = {UNUSED, NONE, NONE, 0, 0, 0},
    [0xe4] = {UNUSED, NONE, NONE, 0, 0, 0},
    [0xe5] = {PUSH, REG_HL, NONE, 1, 4, 4},
    [0xe6] = {AND, REG_A, IMM_8, 2, 2, 2},
    [0xe7] = {RST, PTR_0x20, NONE, 1, 4, 4},
    [0xe8] = {ADD, REG_SP, IMM_8, 2, 4, 4},
    [0xe9] = {JP, PTR_HL, NONE, 1, 2, 2},
    [0xea] = {LD, PTR_16, REG_A, 3, 4, 4},
    [0xeb] = {UNUSED, NONE, NONE, 0, 0, 0},
    [0xec] = {UNUSED, NONE, NONE, 0, 0, 0},
    [0xed] = {UNUSED, NONE, NONE, 0, 0, 0},
    [0xee] = {XOR, REG_A, IMM_8, 2, 2, 2},
    [0xef] = {RST, PTR_0x28, NONE, 1, 4, 4},
    [0xf0] = {LDH, REG_A, PTR_8, 2, 3, 3},
    [0xf1] = {POP, REG_AF, NONE, 1, 3, 3},
    [0xf2] = {LDH, REG_A, PTR_C, 1, 2, 2},
    [0xf3] = {DI, NONE, NONE, 1, 1, 1},
    [0xf4] = {UNUSED, NONE, NONE, 0, 0, 0},
    [0xf5] = {PUSH, REG_AF, NONE, 1, 4, 4},
    [0xf6] = {OR, REG_A, IMM_8, 2, 2, 2},
    [0xf7] = {RST, PTR_0x30, NONE, 1, 4, 4},
    [0xf8] = {LD, REG_HL, IMM_8, 2, 3, 3},
    [0xf9] = {LD, REG_SP, REG_HL, 1, 2, 2},
    [0xfa] = {LD, REG_A, PTR_16, 3, 4, 4},
    [0xfb] = {EI, NONE, NONE, 1, 4, 4},
    [0xfc] = {UNUSED, NONE, NONE, 0, 0, 0},
    [0xfd] = {UNUSED, NONE, NONE, 0, 0, 0},
    [0xfe] = {CP, REG_A, IMM_8, 2, 2, 2},
    [0xff] = {RST, PTR_0x38, NONE, 1, 4, 4},

    // high 256 instructions (0xCB prefixed)
    [0x100] = {RLC, REG_B, NONE, 2, 2, 2},
    [0x101] = {RLC, REG_C, NONE, 2, 2, 2},
    [0x102] = {RLC, REG_D, NONE, 2, 2, 2},
    [0x103] = {RLC, REG_E, NONE, 2, 2, 2},
    [0x104] = {RLC, REG_H, NONE, 2, 2, 2},
    [0x105] = {RLC, REG_L, NONE, 2, 2, 2},
    [0x106] = {RLC, PTR_HL, NONE, 2, 4, 4},
    [0x107] = {RLC, REG_A, NONE, 2, 2, 2},
    [0x108] = {RRC, REG_B, NONE, 2, 2, 2},
    [0x109] = {RRC, REG_C, NONE, 2, 2, 2},
    [0x10a] = {RRC, REG_D, NONE, 2, 2, 2},
    [0x10b] = {RRC, REG_E, NONE, 2, 2, 2},
    [0x10c] = {RRC, REG_H, NONE, 2, 2, 2},
    [0x10d] = {RRC, REG_L, NONE, 2, 2, 2},
    [0x10e] = {RRC, PTR_HL, NONE, 2, 4, 4},
    [0x10f] = {RRC, REG_A, NONE, 2, 2, 2},
    [0x110] = {RL, REG_B, NONE, 2, 2, 2},
    [0x111] = {RL, REG_C, NONE, 2, 2, 2},
    [0x112] = {RL, REG_D, NONE, 2, 2, 2},
    [0x113] = {RL, REG_E, NONE, 2, 2, 2},
    [0x114] = {RL, REG_H, NONE, 2, 2, 2},
    [0x115] = {RL, REG_L, NONE, 2, 2, 2},
    [0x116] = {RL, PTR_HL, NONE, 2, 4, 4},
    [0x117] = {RL, REG_A, NONE, 2, 2, 2},
    [0x118] = {RR, REG_B, NONE, 2, 2, 2},
    [0x119] = {RR, REG_C, NONE, 2, 2, 2},
    [0x11a] = {RR, REG_D, NONE, 2, 2, 2},
    [0x11b] = {RR, REG_E, NONE, 2, 2, 2},
    [0x11c] = {RR, REG_H, NONE, 2, 2, 2},
    [0x11d] = {RR, REG_L, NONE, 2, 2, 2},
    [0x11e] = {RR, PTR_HL, NONE, 2, 4, 4},
    [0x11f] = {RR, REG_A, NONE, 2, 2, 2},
    [0x120] = {SLA, REG_B, NONE, 2, 2, 2},
    [0x121] = {SLA, REG_C, NONE, 2, 2, 2},
    [0x122] = {SLA, REG_D, NONE, 2, 2, 2},
    [0x123] = {SLA, REG_E, NONE, 2, 2, 2},
    [0x124] = {SLA, REG_H, NONE, 2, 2, 2},
    [0x125] = {SLA, REG_L, NONE, 2, 2, 2},
    [0x126] = {SLA, PTR_HL, NONE, 2, 4, 4},
    [0x127] = {SLA, REG_A, NONE, 2, 2, 2},
    [0x128] = {SRA, REG_B, NONE, 2, 2, 2},
    [0x129] = {SRA, REG_C, NONE, 2, 2, 2},
    [0x12a] = {SRA, REG_D, NONE, 2, 2, 2},
    [0x12b] = {SRA, REG_E, NONE, 2, 2, 2},
    [0x12c] = {SRA, REG_H, NONE, 2, 2, 2},
    [0x12d] = {SRA, REG_L, NONE, 2, 2, 2},
    [0x12e] = {SRA, PTR_HL, NONE, 2, 4, 4},
    [0x12f] = {SRA, REG_A, NONE, 2, 2, 2},
    [0x130] = {SWAP, REG_B, NONE, 2, 2, 2},
    [0x131] = {SWAP, REG_C, NONE, 2, 2, 2},
    [0x132] = {SWAP, REG_D, NONE, 2, 2, 2},
    [0x133] = {SWAP, REG_E, NONE, 2, 2, 2},
    [0x134] = {SWAP, REG_H, NONE, 2, 2, 2},
    [0x135] = {SWAP, REG_L, NONE, 2, 2, 2},
    [0x136] = {SWAP, PTR_HL, NONE, 2, 4, 4},
    [0x137] = {SWAP, REG_A, NONE, 2, 2, 2},
    [0x138] = {SRL, REG_B, NONE, 2, 2, 2},
    [0x139] = {SRL, REG_C, NONE, 2, 2, 2},
    [0x13a] = {SRL, REG_D, NONE, 2, 2, 2},
    [0x13b] = {SRL, REG_E, NONE, 2, 2, 2},
    [0x13c] = {SRL, REG_H, NONE, 2, 2, 2},
    [0x13d] = {SRL, REG_L, NONE, 2, 2, 2},
    [0x13e] = {SRL, PTR_HL, NONE, 2, 4, 4},
    [0x13f] = {SRL, REG_A, NONE, 2, 2, 2},
    [0x140] = {BIT, BIT_0, REG_B, 2, 2, 2},
    [0x141] = {BIT, BIT_0, REG_C, 2, 2, 2},
    [0x142] = {BIT, BIT_0, REG_D, 2, 2, 2},
    [0x143] = {BIT, BIT_0, REG_E, 2, 2, 2},
    [0x144] = {BIT, BIT_0, REG_H, 2, 2, 2},
    [0x145] = {BIT, BIT_0, REG_L, 2, 2, 2},
    [0x146] = {BIT, BIT_0, PTR_HL, 2, 3, 3},
    [0x147] = {BIT, BIT_0, REG_A, 2, 2, 2},
    [0x148] = {BIT, BIT_1, REG_B, 2, 2, 2},
    [0x149] = {BIT, BIT_1, REG_C, 2, 2, 2},
    [0x14a] = {BIT, BIT_1, REG_D, 2, 2, 2},
    [0x14b] = {BIT, BIT_1, REG_E, 2, 2, 2},
    [0x14c] = {BIT, BIT_1, REG_H, 2, 2, 2},
    [0x14d] = {BIT, BIT_1, REG_L, 2, 2, 2},
    [0x14e] = {BIT, BIT_1, PTR_HL, 2, 3, 3},
    [0x14f] = {BIT, BIT_1, REG_A, 2, 2, 2},
    [0x150] = {BIT, BIT_2, REG_B, 2, 2, 2},
    [0x151] = {BIT, BIT_2, REG_C, 2, 2, 2},
    [0x152] = {BIT, BIT_2, REG_D, 2, 2, 2},
    [0x153] = {BIT, BIT_2, REG_E, 2, 2, 2},
    [0x154] = {BIT, BIT_2, REG_H, 2, 2, 2},
    [0x155] = {BIT, BIT_2, REG_L, 2, 2, 2},
    [0x156] = {BIT, BIT_2, PTR_HL, 2, 3, 3},
    [0x157] = {BIT, BIT_2, REG_A, 2, 2, 2},
    [0x158] = {BIT, BIT_3, REG_B, 2, 2, 2},
    [0x159] = {BIT, BIT_3, REG_C, 2, 2, 2},
    [0x15a] = {BIT, BIT_3, REG_D, 2, 2, 2},
    [0x15b] = {BIT, BIT_3, REG_E, 2, 2, 2},
    [0x15c] = {BIT, BIT_3, REG_H, 2, 2, 2},
    [0x15d] = {BIT, BIT_3, REG_L, 2, 2, 2},
    [0x15e] = {BIT, BIT_3, PTR_HL, 2, 3, 3},
    [0x15f] = {BIT, BIT_3, REG_A, 2, 2, 2},
    [0x160] = {BIT, BIT_4, REG_B, 2, 2, 2},
    [0x161] = {BIT, BIT_4, REG_C, 2, 2, 2},
    [0x162] = {BIT, BIT_4, REG_D, 2, 2, 2},
    [0x163] = {BIT, BIT_4, REG_E, 2, 2, 2},
    [0x164] = {BIT, BIT_4, REG_H, 2, 2, 2},
    [0x165] = {BIT, BIT_4, REG_L, 2, 2, 2},
    [0x166] = {BIT, BIT_4, PTR_HL, 2, 3, 3},
    [0x167] = {BIT, BIT_4, REG_A, 2, 2, 2},
    [0x168] = {BIT, BIT_5, REG_B, 2, 2, 2},
    [0x169] = {BIT, BIT_5, REG_C, 2, 2, 2},
    [0x16a] = {BIT, BIT_5, REG_D, 2, 2, 2},
    [0x16b] = {BIT, BIT_5, REG_E, 2, 2, 2},
    [0x16c] = {BIT, BIT_5, REG_H, 2, 2, 2},
    [0x16d] = {BIT, BIT_5, REG_L, 2, 2, 2},
    [0x16e] = {BIT, BIT_5, PTR_HL, 2, 3, 3},
    [0x16f] = {BIT, BIT_5, REG_A, 2, 2, 2},
    [0x170] = {BIT, BIT_6, REG_B, 2, 2, 2},
    [0x171] = {BIT, BIT_6, REG_C, 2, 2, 2},
    [0x172] = {BIT, BIT_6, REG_D, 2, 2, 2},
    [0x173] = {BIT, BIT_6, REG_E, 2, 2, 2},
    [0x174] = {BIT, BIT_6, REG_H, 2, 2, 2},
    [0x175] = {BIT, BIT_6, REG_L, 2, 2, 2},
    [0x176] = {BIT, BIT_6, PTR_HL, 2, 3, 3},
    [0x177] = {BIT, BIT_6, REG_A, 2, 2, 2},
    [0x178] = {BIT, BIT_7, REG_B, 2, 2, 2},
    [0x179] = {BIT, BIT_7, REG_C, 2, 2, 2},
    [0x17a] = {BIT, BIT_7, REG_D, 2, 2, 2},
    [0x17b] = {BIT, BIT_7, REG_E, 2, 2, 2},
    [0x17c] = {BIT, BIT_7, REG_H, 2, 2, 2},
    [0x17d] = {BIT, BIT_7, REG_L, 2, 2, 2},
    [0x17e] = {BIT, BIT_7, PTR_HL, 2, 3, 3},
    [0x17f] = {BIT, BIT_7, REG_A, 2, 2, 2},
    [0x180] = {RES, BIT_0, REG_B, 2, 2, 2},
    [0x181] = {RES, BIT_0, REG_C, 2, 2, 2},
    [0x182] = {RES, BIT_0, REG_D, 2, 2, 2},
    [0x183] = {RES, BIT_0, REG_E, 2, 2, 2},
    [0x184] = {RES, BIT_0, REG_H, 2, 2, 2},
    [0x185] = {RES, BIT_0, REG_L, 2, 2, 2},
    [0x186] = {RES, BIT_0, PTR_HL, 2, 4, 4},
    [0x187] = {RES, BIT_0, REG_A, 2, 2, 2},
    [0x188] = {RES, BIT_1, REG_B, 2, 2, 2},
    [0x189] = {RES, BIT_1, REG_C, 2, 2, 2},
    [0x18a] = {RES, BIT_1, REG_D, 2, 2, 2},
    [0x18b] = {RES, BIT_1, REG_E, 2, 2, 2},
    [0x18c] = {RES, BIT_1, REG_H, 2, 2, 2},
    [0x18d] = {RES, BIT_1, REG_L, 2, 2, 2},
    [0x18e] = {RES, BIT_1, PTR_HL, 2, 4, 4},
    [0x18f] = {RES, BIT_1, REG_A, 2, 2, 2},
    [0x190] = {RES, BIT_2, REG_B, 2, 2, 2},
    [0x191] = {RES, BIT_2, REG_C, 2, 2, 2},
    [0x192] = {RES, BIT_2, REG_D, 2, 2, 2},
    [0x193] = {RES, BIT_2, REG_E, 2, 2, 2},
    [0x194] = {RES, BIT_2, REG_H, 2, 2, 2},
    [0x195] = {RES, BIT_2, REG_L, 2, 2, 2},
    [0x196] = {RES, BIT_2, PTR_HL, 2, 4, 4},
    [0x197] = {RES, BIT_2, REG_A, 2, 2, 2},
    [0x198] = {RES, BIT_3, REG_B, 2, 2, 2},
    [0x199] = {RES, BIT_3, REG_C, 2, 2, 2},
    [0x19a] = {RES, BIT_3, REG_D, 2, 2, 2},
    [0x19b] = {RES, BIT_3, REG_E, 2, 2, 2},
    [0x19c] = {RES, BIT_3, REG_H, 2, 2, 2},
    [0x19d] = {RES, BIT_3, REG_L, 2, 2, 2},
    [0x19e] = {RES, BIT_3, PTR_HL, 2, 4, 4},
    [0x19f] = {RES, BIT_3, REG_A, 2, 2, 2},
    [0x1a0] = {RES, BIT_4, REG_B, 2, 2, 2},
    [0x1a1] = {RES, BIT_4, REG_C, 2, 2, 2},
    [0x1a2] = {RES, BIT_4, REG_D, 2, 2, 2},
    [0x1a3] = {RES, BIT_4, REG_E, 2, 2, 2},
    [0x1a4] = {RES, BIT_4, REG_H, 2, 2, 2},
    [0x1a5] = {RES, BIT_4, REG_L, 2, 2, 2},
    [0x1a6] = {RES, BIT_4, PTR_HL, 2, 4, 4},
    [0x1a7] = {RES, BIT_4, REG_A, 2, 2, 2},
    [0x1a8] = {RES, BIT_5, REG_B, 2, 2, 2},
    [0x1a9] = {RES, BIT_5, REG_C, 2, 2, 2},
    [0x1aa] = {RES, BIT_5, REG_D, 2, 2, 2},
    [0x1ab] = {RES, BIT_5, REG_E, 2, 2, 2},
    [0x1ac] = {RES, BIT_5, REG_H, 2, 2, 2},
    [0x1ad] = {RES, BIT_5, REG_L, 2, 2, 2},
    [0x1ae] = {RES, BIT_5, PTR_HL, 2, 4, 4},
    [0x1af] = {RES, BIT_5, REG_A, 2, 2, 2},
    [0x1b0] = {RES, BIT_6, REG_B, 2, 2, 2},
    [0x1b1] = {RES, BIT_6, REG_C, 2, 2, 2},
    [0x1b2] = {RES, BIT_6, REG_D, 2, 2, 2},
    [0x1b3] = {RES, BIT_6, REG_E, 2, 2, 2},
    [0x1b4] = {RES, BIT_6, REG_H, 2, 2, 2},
    [0x1b5] = {RES, BIT_6, REG_L, 2, 2, 2},
    [0x1b6] = {RES, BIT_6, PTR_HL, 2, 4, 4},
    [0x1b7] = {RES, BIT_6, REG_A, 2, 2, 2},
    [0x1b8] = {RES, BIT_7, REG_B, 2, 2, 2},
    [0x1b9] = {RES, BIT_7, REG_C, 2, 2, 2},
    [0x1ba] = {RES, BIT_7, REG_D, 2, 2, 2},
    [0x1bb] = {RES, BIT_7, REG_E, 2, 2, 2},
    [0x1bc] = {RES, BIT_7, REG_H, 2, 2, 2},
    [0x1bd] = {RES, BIT_7, REG_L, 2, 2, 2},
    [0x1be] = {RES, BIT_7, PTR_HL, 2, 4, 4},
    [0x1bf] = {RES, BIT_7, REG_A, 2, 2, 2},
    [0x1c0] = {SET, BIT_0, REG_B, 2, 2, 2},
    [0x1c1] = {SET, BIT_0, REG_C, 2, 2, 2},
    [0x1c2] = {SET, BIT_0, REG_D, 2, 2, 2},
    [0x1c3] = {SET, BIT_0, REG_E, 2, 2, 2},
    [0x1c4] = {SET, BIT_0, REG_H, 2, 2, 2},
    [0x1c5] = {SET, BIT_0, REG_L, 2, 2, 2},
    [0x1c6] = {SET, BIT_0, PTR_HL, 2, 4, 4},
    [0x1c7] = {SET, BIT_0, REG_A, 2, 2, 2},
    [0x1c8] = {SET, BIT_1, REG_B, 2, 2, 2},
    [0x1c9] = {SET, BIT_1, REG_C, 2, 2, 2},
    [0x1ca] = {SET, BIT_1, REG_D, 2, 2, 2},
    [0x1cb] = {SET, BIT_1, REG_E, 2, 2, 2},
    [0x1cc] = {SET, BIT_1, REG_H, 2, 2, 2},
    [0x1cd] = {SET, BIT_1, REG_L, 2, 2, 2},
    [0x1ce] = {SET, BIT_1, PTR_HL, 2, 4, 4},
    [0x1cf] = {SET, BIT_1, REG_A, 2, 2, 2},
    [0x1d0] = {SET, BIT_2, REG_B, 2, 2, 2},
    [0x1d1] = {SET, BIT_2, REG_C, 2, 2, 2},
    [0x1d2] = {SET, BIT_2, REG_D, 2, 2, 2},
    [0x1d3] = {SET, BIT_2, REG_E, 2, 2, 2},
    [0x1d4] = {SET, BIT_2, REG_H, 2, 2, 2},
    [0x1d5] = {SET, BIT_2, REG_L, 2, 2, 2},
    [0x1d6] = {SET, BIT_2, PTR_HL, 2, 4, 4},
    [0x1d7] = {SET, BIT_2, REG_A, 2, 2, 2},
    [0x1d8] = {SET, BIT_3, REG_B, 2, 2, 2},
    [0x1d9] = {SET, BIT_3, REG_C, 2, 2, 2},
    [0x1da] = {SET, BIT_3, REG_D, 2, 2, 2},
    [0x1db] = {SET, BIT_3, REG_E, 2, 2, 2},
    [0x1dc] = {SET, BIT_3, REG_H, 2, 2, 2},
    [0x1dd] = {SET, BIT_3, REG_L, 2, 2, 2},
    [0x1de] = {SET, BIT_3, PTR_HL, 2, 4, 4},
    [0x1df] = {SET, BIT_3, REG_A, 2, 2, 2},
    [0x1e0] = {SET, BIT_4, REG_B, 2, 2, 2},
    [0x1e1] = {SET, BIT_4, REG_C, 2, 2, 2},
    [0x1e2] = {SET, BIT_4, REG_D, 2, 2, 2},
    [0x1e3] = {SET, BIT_4, REG_E, 2, 2, 2},
    [0x1e4] = {SET, BIT_4, REG_H, 2, 2, 2},
    [0x1e5] = {SET, BIT_4, REG_L, 2, 2, 2},
    [0x1e6] = {SET, BIT_4, PTR_HL, 2, 4, 4},
    [0x1e7] = {SET, BIT_4, REG_A, 2, 2, 2},
    [0x1e8] = {SET, BIT_5, REG_B, 2, 2, 2},
    [0x1e9] = {SET, BIT_5, REG_C, 2, 2, 2},
    [0x1ea] = {SET, BIT_5, REG_D, 2, 2, 2},
    [0x1eb] = {SET, BIT_5, REG_E, 2, 2, 2},
    [0x1ec] = {SET, BIT_5, REG_H, 2, 2, 2},
    [0x1ed] = {SET, BIT_5, REG_L, 2, 2, 2},
    [0x1ee] = {SET, BIT_5, PTR_HL, 2, 4, 4},
    [0x1ef] = {SET, BIT_5, REG_A, 2, 2, 2},
    [0x1f0] = {SET, BIT_6, REG_B, 2, 2, 2},
    [0x1f1] = {SET, BIT_6, REG_C, 2, 2, 2},
    [0x1f2] = {SET, BIT_6, REG_D, 2, 2, 2},
    [0x1f3] = {SET, BIT_6, REG_E, 2, 2, 2},
    [0x1f4] = {SET, BIT_6, REG_H, 2, 2, 2},
    [0x1f5] = {SET, BIT_6, REG_L, 2, 2, 2},
    [0x1f6] = {SET, BIT_6, PTR_HL, 2, 4, 4},
    [0x1f7] = {SET, BIT_6, REG_A, 2, 2, 2},
    [0x1f8] = {SET, BIT_7, REG_B, 2, 2, 2},
    [0x1f9] = {SET, BIT_7, REG_C, 2, 2, 2},
    [0x1fa] = {SET, BIT_7, REG_D, 2, 2, 2},
    [0x1fb] = {SET, BIT_7, REG_E, 2, 2, 2},
    [0x1fc] = {SET, BIT_7, REG_H, 2, 2, 2},
    [0x1fd] = {SET, BIT_7, REG_L, 2, 2, 2},
    [0x1fe] = {SET, BIT_7, PTR_HL, 2, 4, 4},
    [0x1ff] = {SET, BIT_7, REG_A, 2, 2, 2},
};

// returns the number of m-cycles elapsed during instruction execution
uint8_t execute_instruction(gb_cpu *cpu)
{
    // the instruction's duration
    uint8_t curr_inst_duration;

    uint8_t inst_code = read_byte(cpu->bus, (cpu->reg->pc)++);
    gb_instruction inst = instruction_table[inst_code];

    // check if we need to access a prefixed instruction
    if (inst.opcode == PREFIX)
    {
        // read the prefixed instruction code and access instruction
        inst_code = read_byte(cpu->bus, (cpu->reg->pc)++);
        inst = instruction_table[0x100 + inst_code];
    }

    switch (inst.opcode)
    {
        case NOP:
            curr_inst_duration = inst.duration;
            break;

        case LD:
            ld(cpu, &inst);
            break;

        case LDH:
            ldh(cpu, &inst);
            break;

        case INC:
            inc(cpu, &inst);
            break;

        case DEC:
            dec(cpu, &inst);
            break;

        case ADD:
            add(cpu, &inst);
            break;

        case ADC:
            adc(cpu, &inst);
            break;

        case SUB:
            sub(cpu, &inst);
            break;

        case SBC:
            sbc(cpu, &inst);
            break;

        case AND:
            and(cpu, &inst);
            break;

        case OR:
            or(cpu, &inst);
            break;

        case RLCA:
            break;

        case RRCA:
            break;

        case STOP:
            break;

        case RLA:
            break;

        case JR:
            break;

        case RRA:
            break;

        case DAA:
            break;

        case CPL:
            break;

        case SCF:
            break;

        case CCF:
            break;

        case HALT:
            break;

        case XOR:
            break;

        case CP:
            break;

        case RET:
            break;

        case POP:
            break;

        case JP:
            break;

        case CALL:
            break;

        case PUSH:
            break;

        case RST:
            break;

        case RETI:
            break;

        case DI:
            break;

        case EI:
            break;

        // CB Opcodes
        case RLC:
            break;

        case RRC:
            break;

        case RL:
            break;

        case RR:
            break;

        case SLA:
            break;

        case SRA:
            break;

        case SWAP:
            break;

        case SRL:
            break;

        case BIT:
            break;

        case RES:
            break;

        case SET:
            break;

        // invalid opcodes are just ignored
        // may lead to undefined behavior
        case UNUSED:
        default:
            curr_inst_duration = 0;
    }
    return curr_inst_duration;
}
