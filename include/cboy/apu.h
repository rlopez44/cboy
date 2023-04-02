#ifndef GB_APU_H
#define GB_APU_H

#include <stdint.h>
#include <SDL_audio.h>
#include <stdbool.h>

/* square wave (envelope and sweep) */
#define NR10_REGISTER 0xff10
#define NR11_REGISTER 0xff11
#define NR12_REGISTER 0xff12
#define NR13_REGISTER 0xff13
#define NR14_REGISTER 0xff14

/* square wave (envelope only) */
#define NR21_REGISTER 0xff16
#define NR22_REGISTER 0xff17
#define NR23_REGISTER 0xff18
#define NR24_REGISTER 0xff19

/* custom wave */
#define NR30_REGISTER 0xff1a
#define NR31_REGISTER 0xff1b
#define NR32_REGISTER 0xff1c
#define NR33_REGISTER 0xff1d
#define NR34_REGISTER 0xff1e

/* noise */
#define NR41_REGISTER 0xff20
#define NR42_REGISTER 0xff21
#define NR43_REGISTER 0xff22
#define NR44_REGISTER 0xff23

/* global to APU */
#define NR50_REGISTER 0xff24
#define NR51_REGISTER 0xff25
#define NR52_REGISTER 0xff26

#define AUDIO_BUFFER_SIZE   1024
#define NUM_CHANNELS        2 // 1 = mono, 2 = stereo
#define AUDIO_SAMPLE_RATE   44100.0
#define GP_CPU_FREQUENCY    4194304.0
#define T_CYCLES_PER_SAMPLE ((uint16_t)(GP_CPU_FREQUENCY / AUDIO_SAMPLE_RATE))

/* number of bytes in wave RAM */
#define WAVE_RAM_SIZE 16

typedef enum APU_CHANNELS {
    CHANNEL_ONE,
    CHANNEL_TWO,
    CHANNEL_THREE,
    CHANNEL_FOUR,
} APU_CHANNELS;

typedef struct apu_pulse_channel {
    uint8_t duty_number;
    uint8_t duty_pos;

    uint8_t length_timer;

    uint16_t wavelength;
    uint16_t wavelength_timer;

    bool length_timer_enable;

    uint8_t initial_volume;
    bool env_incrementing;
    uint8_t env_period;

    uint8_t current_volume;
    uint8_t env_period_timer;

    // sweep variables only used by channel 1
    bool sweep_enabled;
    uint8_t sweep_period;
    uint8_t sweep_period_timer;
    bool sweep_incrementing;
    uint8_t sweep_slope;

    bool enabled, dac_enabled;
} apu_pulse_channel;

typedef struct apu_wave_channel {
    uint16_t length_timer;
    bool length_timer_enable;

    uint8_t output_level;

    uint16_t wavelength;
    uint16_t wavelength_timer;

    // pointer to current nibble in wave RAM
    uint8_t wave_loc;
    uint8_t wave_ram[WAVE_RAM_SIZE];

    bool enabled, dac_enabled;
} apu_wave_channel;

typedef struct apu_noise_channel {
    uint8_t length_timer;
    bool length_timer_enable;

    uint8_t initial_volume;
    uint8_t current_volume;
    uint8_t env_period;
    uint8_t env_period_timer;
    bool env_incrementing;

    uint8_t clock_shift, clock_div_code;
    bool lfsr_width_flag;
    uint16_t lfsr;

    uint16_t wavelength_timer;

    bool enabled, dac_enabled;
} apu_noise_channel;

typedef struct gb_apu {
    SDL_AudioDeviceID audio_dev;
    SDL_AudioSpec audio_spec;

    bool enabled;
    uint8_t panning_info;
    uint16_t sample_timer;

    uint8_t left_volume;
    uint8_t right_volume;

    // VIN is not used by any licensed game
    // See: https://gbdev.io/pandocs/Audio.html?highlight=VIN#architecture
    bool mix_vin_left;
    bool mix_vin_right;

    float sample_buffer[AUDIO_BUFFER_SIZE];
    uint16_t num_samples;

    uint8_t frame_seq_pos;
    uint16_t clock;

    apu_pulse_channel channel_one;
    apu_pulse_channel channel_two;
    apu_wave_channel channel_three;
    apu_noise_channel channel_four;
} gb_apu;

typedef struct gameboy gameboy;

void apu_write(gameboy *gb, uint16_t address, uint8_t value);
uint8_t apu_read(gameboy *gb, uint16_t address);

gb_apu *init_apu(void);
void deinit_apu(gb_apu *apu);

void run_apu(gameboy *gb, uint8_t num_clocks);

#endif /* GB_APU_H */
