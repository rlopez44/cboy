#ifndef GB_MBC_DISPATCH_H
#define GB_MBC_DISPATCH_H

#include <stdint.h>
#include "cboy/mbc.h"
#include "cboy/gameboy.h"

uint8_t no_mbc_read(gameboy *gb, uint16_t address);
uint8_t mbc1_read(gameboy *gb, uint16_t address);
uint8_t mbc3_read(gameboy *gb, uint16_t address);
uint8_t mbc5_read(gameboy *gb, uint16_t address);

void no_mbc_write(gameboy *gb, uint16_t address, uint8_t value);
void mbc1_write(gameboy *gb, uint16_t address, uint8_t value);
void mbc3_write(gameboy *gb, uint16_t address, uint8_t value);
void mbc5_write(gameboy *gb, uint16_t address, uint8_t value);

#endif /* GB_MBC_DISPATCH_H */
