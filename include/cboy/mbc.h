#ifndef GB_MBC_H
#define GB_MBC_H

#include <stdbool.h>
#include <stdint.h>

/* Memory Bank Controller types.
 * See: https://gbdev.io/pandocs/The_Cartridge_Header.html
 */
typedef enum MBC_TYPE {
    UNKNOWN_MBC,
    NO_MBC,
    MBC1,
    MBC2,
    MBC3,
    MBC5,
    MBC6,
    MBC7,
    MMM01,
    HuC1,
    HuC3,
} MBC_TYPE;

/* Memory Bank Controller registers */
typedef enum MBC_REGISTER {
    RAM_ENABLE,
    ROM_BANKNO,
    RAM_BANKNO,
    BANK_MODE,
} MBC_REGISTER;

typedef struct cartridge_mbc1 {
    bool ram_enabled;
    uint8_t rom_bankno;
    uint8_t ram_bankno;
    bool bank_mode;
} cartridge_mbc1;

typedef struct cartridge_mbc3 {
    bool ram_and_rtc_enabled;
    uint8_t rom_bankno;
    uint8_t ram_or_rtc_select;
    uint8_t rtc_latch;

    uint32_t rtc_tick_timer;
    uint8_t rtc_latched_values[5];
    uint8_t rtc_s, rtc_m, rtc_h;
    uint16_t rtc_d;
    bool rtc_halt;
    bool day_carry;
} cartridge_mbc3;

typedef struct cartridge_mbc5 {
    bool ram_enabled;
    uint8_t lsb_rom_bankno;
    bool bit9_rom_bankno;
    uint8_t ram_bankno;
} cartridge_mbc5;

typedef struct cartridge_mbc {
    MBC_TYPE mbc_type;
    union
    {
        cartridge_mbc1 mbc1;
        cartridge_mbc3 mbc3;
        cartridge_mbc5 mbc5;
    };
} cartridge_mbc;

/* Initialize a memory bank controller with the
 * appropriate registers for the given MBC type.
 */
void init_mbc(MBC_TYPE mbc_type, cartridge_mbc *mbc);

// check if the given MBC type is supported
bool mbc_supported(MBC_TYPE mbc_type);

/* utility function for printing out the cartridge MBC type */
void print_mbc_type(MBC_TYPE mbc_type);

/* forward declaration needed for handle_mbc_writes() */
typedef struct gameboy gameboy;

/* Handle reads from cartridge ROM/RAM */
uint8_t cartridge_read(gameboy *gb, uint16_t address);

/* Handle writes to cartridge ROM/RAM */
void cartridge_write(gameboy *gb, uint16_t address, uint8_t value);

/* MBC3 only: tick the RTC by the given number of clocks */
void tick_rtc(gameboy *gb, uint8_t num_clocks);

/* MBC3 only: update the RTC registers by the given number of seconds.
 * Should only be used to update the RTC between emulator sessions.
 */
void fast_forward_rtc(cartridge_mbc3 *mbc, uint64_t num_seconds);

#endif /* GB_MBC_H */
