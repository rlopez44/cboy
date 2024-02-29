#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <SDL_events.h>
#include <SDL_audio.h>
#include <SDL_pixels.h>
#include "cboy/common.h"
#include "cboy/gameboy.h"
#include "cboy/memory.h"
#include "cboy/mbc.h"
#include "cboy/cartridge.h"
#include "cboy/ppu.h"
#include "cboy/interrupts.h"
#include "cboy/instructions.h"
#include "cboy/joypad.h"
#include "cboy/apu.h"
#include "cboy/log.h"

/* 3 seems like a good scale factor */
#define WINDOW_SCALE 3

/* Bit masks to select a bit out of the internal clock
 * counter based on the value of bits 1-0 of TAC. These
 * are arranged such that each value is the index of the
 * appropriate bit mask.
 */
static const uint16_t timer_circuit_bitmasks[4] = {1 << 9, 1 << 3, 1 << 5, 1 << 7};

void report_volume_level(gameboy *gb, bool add_newline)
{
    const char *fmt = "%s\rCurrent volume:%*s%d/100";
    const char *lead = add_newline ? "\n" : "";
    uint8_t spaces;
    if (gb->volume_slider < 10)
        spaces = 3;
    else if (gb->volume_slider < 100)
        spaces = 2;
    else
        spaces = 1;

    LOG_INFO(fmt, lead, spaces, "", gb->volume_slider);
    fflush(stdout);
}

// Stack pop and push operations
void stack_push(gameboy *gb, uint16_t value)
{
    /* NOTE: The stack grows downward (decreasing address).
     *
     * The push operation can be thought of as performing
     * the following imaginary instructions:
     *
     *  DEC SP
     *  LD [SP], HIGH_BYTE(value) ; little-endian, so hi byte first
     *  DEC SP
     *  LD [SP], LOW_BYTE(value)
     */
    write_byte(gb, --(gb->cpu->reg->sp), (uint8_t)(value >> 8));
    write_byte(gb, --(gb->cpu->reg->sp), (uint8_t)(value & 0xff));
}

uint16_t stack_pop(gameboy *gb)
{
    /* The pop operation can be thought of as performing
     * the inverse of the push's imaginary operations:
     *
     *  LD LOW_BYTE(value), [SP] ; little-endian
     *  INC SP
     *  LD HIGH_BYTE(value), [SP]
     *  INC SP
     */
    uint8_t lo = read_byte(gb, (gb->cpu->reg->sp)++);
    uint8_t hi = read_byte(gb, (gb->cpu->reg->sp)++);

    return (uint16_t)(hi << 8) | (uint16_t)lo;
}

/* Verifies the Nintendo logo bitmap located in the
 * ROM file. If this bitmap is incorrect, an error
 * is printed out indicating this game wouldn't
 * run on an actual Game Boy. However, the emulator
 * doesn't actually care if the bitmap is correct.
 */
static bool verify_logo(gameboy *gb)
{
    // The correct bytes for the Game Boy logo
    // See: https://gbdev.io/pandocs/The_Cartridge_Header.html
    const uint8_t nintendo_logo[48] = {
        0xce, 0xed, 0x66, 0x66, 0xcc, 0x0d, 0x00, 0x0b,
        0x03, 0x73, 0x00, 0x83, 0x00, 0x0c, 0x00, 0x0d,
        0x00, 0x08, 0x11, 0x1f, 0x88, 0x89, 0x00, 0x0e,
        0xdc, 0xcc, 0x6e, 0xe6, 0xdd, 0xdd, 0xd9, 0x99,
        0xbb, 0xbb, 0x67, 0x63, 0x6e, 0x0e, 0xec, 0xcc,
        0xdd, 0xdc, 0x99, 0x9f, 0xbb, 0xb9, 0x33, 0x3e,
    };

    // The logo bitmap is located at bytes 0x104-0x133 in the ROM
    uint8_t rom_nintendo_logo[48];
    uint8_t *logo_addr = gb->cart->rom_banks[0] + 0x104;
    memcpy(rom_nintendo_logo, logo_addr, sizeof rom_nintendo_logo);

    bool valid_bitmap = !memcmp(nintendo_logo,
                                rom_nintendo_logo,
                                sizeof nintendo_logo);

    if (!valid_bitmap)
    {
        LOG_INFO("NOTE: The ROM Nintendo logo bitmap is incorrect."
                 " This ROM wouldn't run on a real Game Boy\n\n");
    }

    return valid_bitmap;
}

/* Verifies the cartridge header checksum in
 * the ROM file. Prints out an error if the
 * checksum fails. However, the emulator
 * doesn't actually care if the checksum fails.
 */
static bool verify_checksum(gameboy *gb)
{
    // header checksum is at byte 0x14d of the zeroth ROM bank
    uint8_t *rom0 = gb->cart->rom_banks[0],
            header_checksum = rom0[0x14d];

    // calculate checksum of bytes at addresses 0x134-0x14c
    // See: https://gbdev.io/pandocs/The_Cartridge_Header.html
    int calculated_checksum = 0;
    for (int i = 0x134; i <= 0x14c; ++i)
    {
        calculated_checksum -= rom0[i] + 1;
    }
    // lower byte of the result is compared to the checksum
    calculated_checksum &= 0xff;

    bool valid_checksum = calculated_checksum == header_checksum;

    if (!valid_checksum)
    {
        LOG_INFO("NOTE: The ROM header checksum failed.\n"
                 "Expected: %d\n"
                 "Actual: %d\n"
                 "This ROM wouldn't run on a real Game Boy\n\n",
                 header_checksum,
                 calculated_checksum);
    }

    return valid_checksum;
}

// Initialize the Game Boy's screen
static bool init_screen(gameboy *gb)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        return false;
    }

    // We upscale our window dimensions from the Game Boy's
    // pixel dimensions so that our window isn't super small.
    gb->window = SDL_CreateWindow("Cboy -- A Game Boy Emulator",
                                  SDL_WINDOWPOS_UNDEFINED,
                                  SDL_WINDOWPOS_UNDEFINED,
                                  WINDOW_SCALE * FRAME_WIDTH,
                                  WINDOW_SCALE * FRAME_HEIGHT,
                                  SDL_WINDOW_SHOWN);

    if (gb->window == NULL)
    {
        return false;
    }

    gb->renderer = SDL_CreateRenderer(gb->window, -1, 0);

    if (gb->renderer == NULL)
    {
        SDL_DestroyWindow(gb->window);
        return false;
    }

    /* NOTE: even though we upscaled our window dimensions,
     * we can maintain the correct number of pixels in this
     * texture. This just means that each pixel will be
     * upscaled in size to fill the window.
     */
    gb->screen = SDL_CreateTexture(gb->renderer,
                                   SDL_PIXELFORMAT_XBGR1555,
                                   SDL_TEXTUREACCESS_STREAMING,
                                   FRAME_WIDTH,
                                   FRAME_HEIGHT);

    if (gb->screen == NULL)
    {
        SDL_DestroyRenderer(gb->renderer);
        SDL_DestroyWindow(gb->window);
        return false;
    }

    SDL_UpdateTexture(gb->screen,
                      NULL,
                      gb->ppu->frame_buffer,
                      FRAME_WIDTH * sizeof gb->ppu->frame_buffer[0]);
    SDL_RenderClear(gb->renderer);
    SDL_RenderCopy(gb->renderer, gb->screen, NULL, NULL);
    SDL_RenderPresent(gb->renderer);

    return true;
}

/* Attempts to load the given boot ROM into the emulator
 * so that it can be played before starting the game.
 *
 * If The boot ROM can't be read (missing file etc.)
 * or it is smaller than 256 bytes in size a warning message
 * is printed out, but the emulator will continue
 * to run without using the boot ROM.
 *
 * On successful boot ROM load, the boot ROM enable register
 * is reset and the CPU's program counter is set to 0x00
 * (the boot ROM is mapped to the first 256 bytes of ROM bank 0).
 */
static void maybe_load_bootrom(gameboy *gb, const char *bootrom)
{
    if (gb->run_mode == GB_CGB_MODE)
    {
        LOG_INFO("Note: boot ROM playback not yet supported for GBC games. Skipping...\n\n");
        gb->boot_rom_disabled = true;
        return;
    }

    FILE *bootrom_file = fopen(bootrom, "rb");
    if (bootrom_file == NULL)
    {
        LOG_ERROR("Unable to load the boot ROM (incorrect path?).\n");
    }
    else
    {
        size_t nbytes_read = fread(gb->boot_rom, sizeof(uint8_t), BOOT_ROM_SIZE, bootrom_file);
        if (BOOT_ROM_SIZE != nbytes_read)
        {
            // either an I/O error occurred, or there weren't enough bytes in the file
            if (ferror(bootrom_file))
                LOG_ERROR("Unable to read from the boot ROM (I/O error).\n");

            // the ROM was too small
            LOG_ERROR("The specified boot ROM is only %zu "
                    "bytes large (expected %d bytes).\n",
                    nbytes_read,
                    BOOT_ROM_SIZE);
        }
        else
        {
            // succeeded in reading in the boot ROM
            gb->run_boot_rom = true;
        }

        fclose(bootrom_file);
    }

    if (gb->run_boot_rom)
    {
        LOG_INFO("Boot ROM loaded successfully.\n\n");
        // set the program counter to the beginning of the boot ROM
        gb->cpu->reg->pc = 0x0000;
    }
    else
    {
        LOG_INFO("The emulator will continue without using a boot ROM.\n\n");
        gb->boot_rom_disabled = true;
    }
}

// Determine whether to run in DMG or CGB mode
static void determine_and_report_run_mode(gameboy *gb, bool force_dmg)
{
    if (force_dmg)
    {
        gb->run_mode = GB_DMG_MODE;
        LOG_INFO("GB Mode: monochrome Game Boy (forced)\n");
    }
    else switch (gb->cart->rom_banks[0][0x143] & 0xbf) // hardware ignores bit 6
    {
        case 0x80:
            gb->run_mode = GB_CGB_MODE;
            LOG_INFO("GB Mode: Game Boy Color\n");
            break;

        default:
            gb->run_mode = GB_DMG_MODE;
            LOG_INFO("GB Mode: monochrome Game Boy\n");
            break;
    }
}

/* Allocates memory for the Game Boy struct
 * and initializes the Game Boy and its components.
 * Loads the ROM file into the emulator.
 *
 * If initialization fails then NULL is returned
 * and an error message is printed out.
 */
gameboy *init_gameboy(const char *rom_file_path, const char *bootrom, bool force_dmg)
{
    gameboy *gb = calloc(1, sizeof(gameboy));

    if (gb == NULL)
    {
        LOG_ERROR("Not enough memory to initialize the emulator\n");
        return NULL;
    }

    gb->is_on = true;
    gb->audio_sync_signal = true;
    gb->volume_slider = 100;
    gb->throttle_fps = true;
    gb->run_mode = GB_DMG_MODE; // updated once game ROM is read in
    gb->tac = 0xf8;

    gb->joypad = init_joypad();
    gb->cart = init_cartridge();
    gb->apu = init_apu();
    gb->memory = init_memory_map();

    bool all_alloc = gb->joypad
                     && gb->cart
                     && gb->apu
                     && gb->memory;
    if (!all_alloc)
        goto init_error;

    FILE *rom_file = fopen(rom_file_path, "rb");

    if (rom_file == NULL)
    {
        LOG_ERROR("Failed to open the ROM file (incorrect path?)\n");
        goto init_error;
    }

    ROM_LOAD_STATUS load_status = load_rom(gb->cart, rom_file);
    fclose(rom_file);

    if (load_status != ROM_LOAD_SUCCESS)
    {
        if (load_status == MALFORMED_ROM)
            LOG_ERROR("ROM file is incorrectly formatted\n");
        else if (load_status == ROM_LOAD_ERROR)
            LOG_ERROR("Failed to load the ROM into the emulator (I/O or memory error)\n");

        goto init_error;
    }

    determine_and_report_run_mode(gb, force_dmg);

    gb->cpu = init_cpu(gb->run_mode);
    gb->ppu = init_ppu(gb->run_mode);

    if (!gb->cpu || !gb->ppu)
        goto init_error;

    maybe_import_cartridge_ram(gb->cart, rom_file_path);

    // finish initializing I/O registers
    if (gb->run_mode == GB_CGB_MODE)
    {
        gb->speed_switch_armed = false;
        gb->double_speed = false;
        gb->key0 = gb->cart->rom_banks[0][0x0143];
        gb->svbk = 0xff;
        gb->vbk = 0xfe;
        gb->vram_dma_source = gb->vram_dma_dest = 0xffff;
        gb->vram_dma_length = 0;
        gb->gdma_running = false;
        gb->hdma_running = false;
    }

    /* Load the boot ROM into the emulator if it was passed in.
     * We need to do this after the memory map is initialized
     * because, if no suitable boot ROM is given, we need to
     * write 1 to memory adress 0xff50 to "disable" the boot
     * ROM (to be faithful to the hardware's behavior).
     */
    if (bootrom != NULL)
        maybe_load_bootrom(gb, bootrom);
    else
        gb->boot_rom_disabled = true;

    // screen must be initialized after the PPU
    if (!init_screen(gb))
        goto init_error;

    verify_logo(gb);
    verify_checksum(gb);

    return gb;

init_error:
    free_gameboy(gb);
    return NULL;
}

/* Free the allocated memory for the Game Boy struct */
void free_gameboy(gameboy *gb)
{
    free_memory_map(gb->memory);
    free_cpu(gb->cpu);
    unload_cartridge(gb->cart);
    free_ppu(gb->ppu);
    free_joypad(gb->joypad);
    deinit_apu(gb->apu);

    if (gb->screen)
        SDL_DestroyTexture(gb->screen);

    if (gb->renderer)
        SDL_DestroyRenderer(gb->renderer);

    if (gb->window)
        SDL_DestroyWindow(gb->window);

    SDL_Quit();
    free(gb);
}

/* CGB only: check if a CPU speed switch should be performed */
bool maybe_switch_speed(gameboy *gb)
{
    if (!gb->speed_switch_armed)
        return false;

    // the internal clock counter is reset on speed switch
    timing_related_write(gb, DIV_REGISTER, 0);
    gb->double_speed = !gb->double_speed;
    gb->speed_switch_armed = false;

    return true;
}

/*
 * Check whether VRAM DMA should occur.
 * On hardware this check is performed
 * by the CPU during each opcode fetch.
 *
 * General-Purpose VRAM DMA is triggered
 * on write to HDMA5.
 *
 * HBLANK VRAM DMA is triggered on the
 * rising edge of the HBLANK mode signal.
 */
static bool check_vram_dma_condition(gameboy *gb)
{
    bool prev_hblank_signal = gb->hblank_signal;
    gb->hblank_signal = !(gb->ppu->stat & 0x3);
    bool do_hdma = gb->hdma_running && !prev_hblank_signal && gb->hblank_signal;

    return gb->gdma_running || do_hdma;
}

// Transfer 0x10 bytes of data as part of VRAM DMA
static void vram_dma_transfer_chunk(gameboy *gb)
{
    uint8_t value;
    // transfer length is always a multiple of 0x10
    for (int i = 0; i < 0x10; ++i)
    {
        if (gb->vram_dma_source <= 0x7fff || (gb->vram_dma_source >= 0xa000 && gb->vram_dma_source <= 0xbfff))
            value = cartridge_read(gb, gb->vram_dma_source);
        else if (gb->vram_dma_source >= 0xc000 && gb->vram_dma_source <= 0xdfff)
            value = ram_read(gb, gb->vram_dma_source);
        else // reading VRAM during vram_dma writes garbage to VRAM
            value = 0xa5; // 0b1010_0101

        ram_write(gb, gb->vram_dma_dest, value);

        ++gb->vram_dma_dest;
        ++gb->vram_dma_source;
    }

    gb->vram_dma_length -= 0x10;

    if (!gb->vram_dma_length)
    {
        gb->hdma_running = false;
        gb->gdma_running = false;
    }
}

/*
 * Handle writes to the following I/O registers:
 * KEY0, KEY1, VBK, SVBK, HDMA[1-5]
 */
void cgb_core_io_write(gameboy *gb, uint16_t address, uint8_t value)
{
    switch (address)
    {
        case KEY1_REGISTER:
            gb->speed_switch_armed = value & 1;
            break;

        case VBK_REGISTER:
            gb->vbk = 0xfe | (value & 1);
            break;

        case SVBK_REGISTER:
            gb->svbk = 0xf8 | (value & 0x7);
            break;

        case HDMA1_REGISTER:
            // source must be in ROM, cartridge RAM, or WRAM
            if (value > 0x7f && !(value >= 0xa0 && value <= 0xdf))
            {
                LOG_ERROR("Invalid HDMA source high: %02x\n", value);
                exit(1);
            }

            gb->vram_dma_source = (gb->vram_dma_source & 0x00f0) | (uint16_t)value << 8;
            break;

        case HDMA2_REGISTER:
            gb->vram_dma_source = (gb->vram_dma_source & 0xff00) | (value & 0xf0);
            break;

        case HDMA3_REGISTER:
            gb->vram_dma_dest = 0x8000 | (gb->vram_dma_dest & 0x00f0) | (uint16_t)(value & 0x1f) << 8;
            break;

        case HDMA4_REGISTER:
            gb->vram_dma_dest = 0x8000 | (gb->vram_dma_dest & 0x1f00) | (value & 0xf0);
            break;

        case HDMA5_REGISTER:
            // HBLANK DMA can be canceled before completion
            if (gb->hdma_running)
                gb->hdma_running = value & 0x80;
            else if (value & 0x80)
                gb->hdma_running = true;
            else
                gb->gdma_running = true;

            gb->vram_dma_length = ((value & 0x7f) + 1) << 4;
            break;

        default:
            break;
    }
}

/*
 * Handle reads from the following I/O registers:
 * KEY0, KEY1, VBK, SVBK, HDMA[1-5]
 */
uint8_t cgb_core_io_read(gameboy *gb, uint16_t address)
{
    uint8_t value;
    switch (address)
    {
        case KEY1_REGISTER:
            value = 0x7e | (gb->double_speed << 7) | gb->speed_switch_armed;
            break;

        case VBK_REGISTER:
            value = gb->vbk;
            break;

        case SVBK_REGISTER:
            value = gb->svbk;
            break;

        case HDMA5_REGISTER:
            // only time this register can be read from while
            // HDMA is still active is if it's an HBLANK DMA
            if (!gb->vram_dma_length)
                value = 0xff;
            else
                value = gb->hdma_running << 7 | ((gb->vram_dma_length >> 4) - 1);
            break;

        default:
            value = 0xff;
            break;
    }

    return value;
}

/* Increment the TIMA register, including handling its
 * overflow behavior. When TIMA overflows the value of
 * TMA is loaded and a timer interrupt is requested.
 *
 * TODO: when TIMA overflows, it's value should actually
 * be set to 0x00 for one m-cycle (four CPU clocks) before
 * loading the value of TMA and requesting a timer interrupt.
 */
void increment_tima(gameboy *gb)
{
    gb->tima += 1;
    if (!gb->tima)
    {
        gb->tima = gb->tma;
        request_interrupt(gb, TIMER);
    }
}

/* Increment the Game Boy's internal clock counter
 * by the given number of clocks. Also handles
 * updating the DIV and TIMA registers as needed.
 *
 * Because the DIV register is simply the upper
 * byte of this internal counter mapped to memory
 * address 0xff04, incrementing the counter also
 * increments the DIV register as needed (every 256
 * clock cycles).
 *
 * Because the TIMA register (memory address 0xff05)
 * is also a timer, we must increment it as needed
 * when the internal clock counter is incremented. The
 * TIMA register can be enabled/disabled via bit 2 of
 * the TAC register (memory address 0xff07). If enabled,
 * The frequency at which this timer increments is
 * specified by bits 1 and 0 of TAC. These frequencies
 * are specified below:
 *
 * TIMA update frequencies
 * ~~~~~~~~~~~~~~~~~~~~~~~
 * Bits 1 and 0 of TAC:
 *     00: CPU Clock / 1024
 *     01: CPU Clock / 16
 *     10: CPU Clock / 64
 *     11: CPU Clock / 256
 *
 * When TIMA overflows, its value is reset to the value
 * specified in the TMA register (memory address 0xff06)
 * and the Timer Interrupt bit in the IF register is set.
 *
 * NOTE: because this emulation is currently not cycle-accurate,
 * the timings of these increments are not exact. This might
 * cause some bugs for timer-related events in the games
 * being played.
 *
 * TODO: explore ways to improve the timing of these increments
 */
void increment_clock_counter(gameboy *gb, uint16_t num_clocks)
{
    bool tima_enabled = gb->tac & 0x4;

    // the number of CPU clock ticks between TIMA increments
    uint16_t tima_tick_interval;
    switch (gb->tac & 0x3)
    {
        case 0x0:
            tima_tick_interval = 0x400;
            break;
        case 0x1:
            tima_tick_interval = 0x10;
            break;
        case 0x2:
            tima_tick_interval = 0x40;
            break;
        case 0x3:
            tima_tick_interval = 0x100;
            break;
    }

    // increment the internal clock counter one tick at a time
    while (num_clocks)
    {
        ++(gb->clock_counter);
        --num_clocks;

        // check if TIMA needs incrementing
        if (tima_enabled && !(gb->clock_counter % tima_tick_interval))
        {
            increment_tima(gb);
        }
    }
}

/* Handle writes to the timing-related registers (DIV, TIMA, TMA, TAC)
 * -------------------------------------------------------------------
 * To determine when to increment TIMA, the Game Boy's timer circuit
 * selects a certain bit of the internal clock counter (see below),
 * performs a logical AND between this bit and the TIMA enable bit of
 * TAC (bit 2), then monitors when this signal switches from 1 to 0.
 *
 * The internal clock counter bit is selected based on the value of
 * bits 1-0 of TAC, such that its flips from 1 to 0 occur at the
 * current TIMA increment frequency:
 *
 *     > Bits 1 and 0 of TAC:
 *           00: Bit 9 of internal clock counter (freq = CPU Clock / 1024)
 *           01: Bit 3 of internal clock counter (freq = CPU Clock / 16)
 *           10: Bit 5 of internal clock counter (freq = CPU Clock / 64)
 *           11: Bit 7 of internal clock counter (freq = CPU Clock / 256)
 *
 * Given the manner in which the monitored signal is constructed, the
 * following conditions cause the 1 -> 0 signal switch to occur:
 *
 *    1. The selected bit of the internal clock counter flips from
 *       1 to 0 while TIMA is enabled (bit 2 of TAC is 1), either
 *       because the clock counter incremented or the DIV register
 *       was written to (here we're interested in the latter).
 *
 *    2. TIMA is currently enabled and a write to TAC occurs that
 *       disables it (i.e. bit 2 of TAC flips from 1 to 0) while the
 *       selected bit of the internal clock counter is 1.
 *
 *    3. TIMA is currently enabled, a write to TAC occurs that switches
 *       the TIMA increment frequency, and this causes the circuit to
 *       switch from a selected bit that is 1, to one that is 0.
 *
 * In this function we check if any of these conditions occur so
 * that we increment TIMA appropriately during the write to memory.
 *
 * See: https://gbdev.io/pandocs/Timer_and_Divider_Registers.html
 */
void timing_related_write(gameboy *gb, uint16_t address, uint8_t value)
{
    // the bit mask used by the timer circuit
    uint16_t bitmask = timer_circuit_bitmasks[gb->tac & 0x3];

    bool selected_bit_is_set = gb->clock_counter & bitmask,
         tima_enabled        = gb->tac & 0x4,
         writing_to_tac      = address == TAC_REGISTER,          // TIMA frequency might change
         disabling_tima      = writing_to_tac && !(value & 0x4), // TIMA will be disabled by write to TAC
         resetting_counter   = address == DIV_REGISTER;

    /********** Check if we need to increment TIMA **********/

    // condition 1 or condition 2 for TIMA increment is met
    if ((disabling_tima || resetting_counter) && tima_enabled && selected_bit_is_set)
    {
        increment_tima(gb);
    }
    // check if condition 3 for TIMA increment is met
    else if (tima_enabled && writing_to_tac)
    {
        // condition 3 is met if we're switching from
        // a bit that is 1 to one that is 0
        bitmask = timer_circuit_bitmasks[value & 0x3];
        bool new_selected_bit_is_set = gb->clock_counter & bitmask;

        if (selected_bit_is_set && !new_selected_bit_is_set)
        {
            increment_tima(gb);
        }
    }

    /********** Perform the actual writes **********/
    switch (address)
    {
        // writing to DIV resets the internal clock counter
        case DIV_REGISTER:
            gb->clock_counter = 0;
            break;

        case TIMA_REGISTER:
            gb->tima = value;
            break;

        case TMA_REGISTER:
            gb->tma = value;
            break;

        // only the lower 3 bits of TAC can be written to
        case TAC_REGISTER:
            gb->tac = 0xf8 | (value & 0x07);
            break;

        default:
            LOG_ERROR("Expected write to timing-related address."
                      " Got: %04x\n",
                      address);
            exit(1);
    }
}

uint8_t timing_related_read(gameboy *gb, uint16_t address)
{
    uint8_t value;
    switch (address)
    {
        // DIV maps to the upper byte of the internal clock counter
        case DIV_REGISTER:
            value = gb->clock_counter >> 8;
            break;

        case TIMA_REGISTER:
            value = gb->tima;
            break;

        case TMA_REGISTER:
            value = gb->tma;
            break;

        case TAC_REGISTER:
            value = gb->tac;
            break;

        default:
            LOG_ERROR("Expected read from timing-related address."
                      " Got: %04x\n",
                      address);
            exit(1);
    }

    return value;
}

/* Check if a DMA transfer needs to be performed
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * To emulate the DMA transfer timing, we wait until the
 * number of clocks that a DMA transfer takes has elapsed
 * since the DMA register was written to, then we perform
 * the DMA transfer all at once. This works because the
 * CPU only has access to HRAM while the DMA process is
 * supposed to be occurring.
 *
 * The transfer takes 160 m-cycles (640 clocks)
 */
static void dma_transfer_check(gameboy *gb, uint8_t num_clocks)
{
    if (gb->dma_requested)
    {
        gb->dma_counter += num_clocks;
        if (gb->dma_counter >= 640)
        {
            LOG_DEBUG("Performing DMA Transfer\n");
            dma_transfer(gb);
            gb->dma_requested = false;
            gb->dma_counter = 0;
        }
    }

}

// check if we can exit the HALT instruction
static void check_halt_wakeup(gameboy *gb)
{
    // we exit if an interrupt is pending
    if (pending_interrupts(gb))
    {
        LOG_DEBUG("Exiting HALTed state\n");
        gb->cpu->is_halted = false;
    }
}

/* Poll emulator input.
 * Should be called once per frame.
 */
static inline void poll_input(gameboy *gb)
{
    SDL_Event event;
    while(SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            case SDL_QUIT:
                gb->is_on = false;
                break;

            case SDL_KEYDOWN:
            case SDL_KEYUP:
                handle_keypress(gb, &event.key);
                break;

            default:
                break;
        }
    }
}

// wait for half the audio buffer to be
// consumed before resuming emulation
static inline void throttle_emulation(gameboy *gb)
{
    bool wait = true;
    do
    {
        SDL_Delay(1);
        SDL_LockAudioDevice(gb->apu->audio_dev);
        wait = gb->apu->num_frames > AUDIO_BUFFER_FRAME_SIZE / 2;
        SDL_UnlockAudioDevice(gb->apu->audio_dev);
    } while (wait);
}

// run the emulator
void run_gameboy(gameboy *gb)
{
    int num_clocks;
    while (gb->is_on)
    {
#ifdef DEBUG
        // print CPU register contents before each instruction
        if (!gb->cpu->is_halted)
            print_registers(gb);
#endif

        // number of CPU clock ticks this iteration of the event loop
        num_clocks = 0;

        if (gb->run_mode == GB_CGB_MODE && check_vram_dma_condition(gb))
        {
            // HDMA transfers 0x10 bytes in 8 normal-speed m-cycles
            vram_dma_transfer_chunk(gb);
            if (gb->double_speed)
                num_clocks += 8 * 8;
            else
                num_clocks += 4 * 8;
        }
        else if (gb->cpu->is_halted)
        {
            // same number of CPU clock ticks as a NOP
            // See: https://gbdev.io/pandocs/CPU_Instruction_Set.html#cpu-control-instructions
            num_clocks += 4;
            check_halt_wakeup(gb);
        }
        else
        {
            // number of CPU clock ticks, given number of m-cycles
            num_clocks += 4 * execute_instruction(gb);
        }

        increment_clock_counter(gb, num_clocks);

        dma_transfer_check(gb, num_clocks);

        // PPU and APU always run at normal speed (RTC as well)
        if (gb->run_mode == GB_CGB_MODE && gb->double_speed)
            num_clocks /= 2;

        if (gb->cart->has_rtc)
            tick_rtc(gb, num_clocks);

        run_apu(gb, num_clocks);

        run_ppu(gb, num_clocks);

        if (gb->frame_presented_signal)
        {
            gb->frame_presented_signal = false;
            poll_input(gb);
        }

        if (gb->audio_sync_signal)
        {
            gb->audio_sync_signal = false;
            if (gb->throttle_fps)
                throttle_emulation(gb);
        }
    }
}
