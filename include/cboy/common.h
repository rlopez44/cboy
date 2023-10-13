#ifndef GB_COMMON_H
#define GB_COMMON_H

/* 16 KB (2^14 bytes) */
#define ROM_BANK_SIZE 16384

/* 1 KB = 2^10 bytes */
#define KB 1024

/* Hz */
#define GB_CPU_FREQUENCY 4194304

/********** Game Boy Memory-Mapped I/O Registers **********/
/* joypad */
#define JOYP_REGISTER 0xff00

/* timer registers */
#define DIV_REGISTER  0xff04
#define TIMA_REGISTER 0xff05
#define TMA_REGISTER  0xff06
#define TAC_REGISTER  0xff07

/* interrupt flags */
#define IF_REGISTER   0xff0f

/* APU - square wave (envelope and sweep) */
#define NR10_REGISTER 0xff10
#define NR11_REGISTER 0xff11
#define NR12_REGISTER 0xff12
#define NR13_REGISTER 0xff13
#define NR14_REGISTER 0xff14

/* APU - square wave (envelope only) */
#define NR21_REGISTER 0xff16
#define NR22_REGISTER 0xff17
#define NR23_REGISTER 0xff18
#define NR24_REGISTER 0xff19

/* APU - custom wave */
#define NR30_REGISTER 0xff1a
#define NR31_REGISTER 0xff1b
#define NR32_REGISTER 0xff1c
#define NR33_REGISTER 0xff1d
#define NR34_REGISTER 0xff1e

/* APU - noise */
#define NR41_REGISTER 0xff20
#define NR42_REGISTER 0xff21
#define NR43_REGISTER 0xff22
#define NR44_REGISTER 0xff23

/* global to APU */
#define NR50_REGISTER 0xff24
#define NR51_REGISTER 0xff25
#define NR52_REGISTER 0xff26

/* PPU-related registers */
#define LCDC_REGISTER 0xff40
#define STAT_REGISTER 0xff41
#define SCY_REGISTER  0xff42
#define SCX_REGISTER  0xff43
#define LY_REGISTER   0xff44
#define LYC_REGISTER  0xff45
#define DMA_REGISTER  0xff46
#define BGP_REGISTER  0xff47
#define OBP0_REGISTER 0xff48
#define OBP1_REGISTER 0xff49
#define WY_REGISTER   0xff4a
#define WX_REGISTER   0xff4b

/* interrupt enable */
#define IE_REGISTER   0xffff

enum GAMEBOY_MODE {
    GB_DMG_MODE,
    GB_CGB_MODE,
};

#endif /* GB_COMMON_H */
