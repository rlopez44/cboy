#ifndef GB_COMMON_H
#define GB_COMMON_H

/* 16 KB (2^14 bytes) */
#define ROM_BANK_SIZE 16384

/* 1 KB = 2^10 bytes */
#define KB 1024

/* Hz */
#define GB_CPU_FREQUENCY 4194304

enum GAMEBOY_MODE {
    GB_DMG_MODE,
    GB_CGB_MODE,
};

/* Game Boy Memory-Mapped I/O Registers */
enum GB_MMIO_REGISTER {
    /* joypad */
    JOYP_REGISTER  = 0xff00,

    /* timer registers */
    DIV_REGISTER   = 0xff04,
    TIMA_REGISTER  = 0xff05,
    TMA_REGISTER   = 0xff06,
    TAC_REGISTER   = 0xff07,

    /* interrupt flags */
    IF_REGISTER    = 0xff0f,

    /* APU - square wave (envelope and sweep) */
    NR10_REGISTER  = 0xff10,
    NR11_REGISTER  = 0xff11,
    NR12_REGISTER  = 0xff12,
    NR13_REGISTER  = 0xff13,
    NR14_REGISTER  = 0xff14,

    /* APU - square wave (envelope only) */
    NR21_REGISTER  = 0xff16,
    NR22_REGISTER  = 0xff17,
    NR23_REGISTER  = 0xff18,
    NR24_REGISTER  = 0xff19,

    /* APU - custom wave */
    NR30_REGISTER  = 0xff1a,
    NR31_REGISTER  = 0xff1b,
    NR32_REGISTER  = 0xff1c,
    NR33_REGISTER  = 0xff1d,
    NR34_REGISTER  = 0xff1e,

    /* APU - noise */
    NR41_REGISTER  = 0xff20,
    NR42_REGISTER  = 0xff21,
    NR43_REGISTER  = 0xff22,
    NR44_REGISTER  = 0xff23,

    /* global to APU */
    NR50_REGISTER  = 0xff24,
    NR51_REGISTER  = 0xff25,
    NR52_REGISTER  = 0xff26,

    /* PPU-related registers */
    LCDC_REGISTER  = 0xff40,
    STAT_REGISTER  = 0xff41,
    SCY_REGISTER   = 0xff42,
    SCX_REGISTER   = 0xff43,
    LY_REGISTER    = 0xff44,
    LYC_REGISTER   = 0xff45,
    DMA_REGISTER   = 0xff46,
    BGP_REGISTER   = 0xff47,
    OBP0_REGISTER  = 0xff48,
    OBP1_REGISTER  = 0xff49,
    WY_REGISTER    = 0xff4a,
    WX_REGISTER    = 0xff4b,

    /* CGB-only registers */
    KEY0_REGISTER  = 0xff4c,
    KEY1_REGISTER  = 0xff4d,
    VBK_REGISTER   = 0xff4f,

    /* Boot ROM disable */
    BRD_REGISTER   = 0xff50,

    /* CGB-only registers cont. */
    HDMA1_REGISTER = 0xff51,
    HDMA2_REGISTER = 0xff52,
    HDMA3_REGISTER = 0xff53,
    HDMA4_REGISTER = 0xff54,
    HDMA5_REGISTER = 0xff55,
    BCPS_REGISTER  = 0xff68,
    BCPD_REGISTER  = 0xff69,
    OCPS_REGISTER  = 0xff6a,
    OCPD_REGISTER  = 0xff6b,
    OPRI_REGISTER  = 0xff6c,
    SVBK_REGISTER  = 0xff70,

    /* interrupt enable */
    IE_REGISTER    = 0xffff,
};

#endif /* GB_COMMON_H */
