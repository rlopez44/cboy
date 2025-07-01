#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <SDL3/SDL.h>
#include "cboy/common.h"
#include "cboy/gameboy.h"
#include "cboy/apu.h"
#include "cboy/log.h"
#include "cboy/mbc.h"

static const uint8_t duty_cycles[4*8] = {
    0, 0, 0, 0, 0, 0, 0, 1, // 12.5%
    1, 0, 0, 0, 0, 0, 0, 1, // 25%
    1, 0, 0, 0, 0, 1, 1, 1, // 50%
    0, 1, 1, 1, 1, 1, 1, 0, // 75%
};

static void init_pulse_channel(apu_pulse_channel *chan, APU_CHANNELS channelno);
static void init_wave_channel(apu_wave_channel *chan);
static void init_noise_channel(apu_noise_channel *chan);
static void sample_audio(gameboy *gb);
static void tick_frame_sequencer(gb_apu *apu);
static void trigger_channel(gb_apu *apu, APU_CHANNELS channel);

static void SDLCALL audio_callback(void *userdata,
                                   SDL_AudioStream *stream,
                                   int additional_amount,
                                   int total_amount);

static inline void tick_channels(gb_apu *apu);

gb_apu *init_apu(void)
{
    gb_apu *apu = malloc(sizeof(gb_apu));
    if (apu == NULL)
        return NULL;

    apu->audio_stream = NULL;
    apu->enabled = false;
    apu->panning_info = 0;
    apu->left_volume = 0x7;
    apu->right_volume = 0x7;
    apu->mix_vin_left = false;
    apu->mix_vin_right = false;
    apu->sample_timer = T_CYCLES_PER_SAMPLE;
    apu->frame_seq_pos = 0;
    apu->clock = 0;

    for (uint8_t i = 0; i < 4; ++i)
        apu->curr_channel_samples[i] = 0;

    // sample buffer initialized full of silence
    for (uint16_t i = 0; i < AUDIO_BUFFER_SAMPLE_SIZE; ++i)
        apu->sample_buffer[i] = 0;

    apu->num_frames = AUDIO_BUFFER_FRAME_SIZE;
    apu->frame_start = 0;
    apu->frame_end = 0;

    init_pulse_channel(&apu->channel_one, CHANNEL_ONE);
    init_pulse_channel(&apu->channel_two, CHANNEL_TWO);
    init_wave_channel(&apu->channel_three);
    init_noise_channel(&apu->channel_four);

    if (!SDL_Init(SDL_INIT_AUDIO))
        goto init_error;

    const SDL_AudioSpec audio_spec = {
        .freq = AUDIO_FRAME_RATE,
        .format = SDL_AUDIO_F32,
        .channels = NUM_CHANNELS,
    };

    apu->audio_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                                                  &audio_spec,
                                                  audio_callback,
                                                  apu);

    if (!apu->audio_stream)
        goto init_error;

    SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(apu->audio_stream));

    return apu;

init_error:
    deinit_apu(apu);
    LOG_ERROR("Failed to fully initialize audio: %s\n",
              SDL_GetError());
    exit(1);
}

void deinit_apu(gb_apu *apu)
{
    if (apu == NULL)
        return;

    if (apu->audio_stream)
        SDL_DestroyAudioStream(apu->audio_stream);

    free(apu);
}

void apu_write(gameboy *gb, uint16_t address, uint8_t value)
{
    gb_apu *apu = gb->apu;

    // write to channel 3 wave RAM
    if (address >= 0xff30 && address <= 0xff3f)
    {
        apu->channel_three.wave_ram[address - 0xff30] = value;
        return;
    }

    switch (address)
    {
        case NR10_REGISTER:
        {
            apu_pulse_channel *chan = &apu->channel_one;
            chan->sweep_slope = value & 0x7;
            // whether *frequency* is increasing (i.e., wavelength decreasing)
            chan->sweep_decrementing = (value >> 3) & 1;
            chan->sweep_period = (value >> 4) & 0x7;
            chan->sweep_enabled = chan->sweep_period;
            break;
        }

        case NR11_REGISTER:
        case NR21_REGISTER:
        {
            apu_pulse_channel *chan;
            if (address == NR11_REGISTER)
                chan = &apu->channel_one;
            else
                chan = &apu->channel_two;

            chan->length_timer = 64 - (value & 0x3f);
            chan->duty_number = (value >> 6) & 0x3;
            break;
        }

        case NR12_REGISTER:
        case NR22_REGISTER:
        {
            apu_pulse_channel *chan;
            if (address == NR12_REGISTER)
                chan = &apu->channel_one;
            else
                chan = &apu->channel_two;

            chan->initial_volume = (value >> 4) & 0xf;
            chan->env_incrementing = (value >> 3) & 1;
            chan->env_period = value & 0x7;
            chan->dac_enabled = value & 0xf8;
            if (!chan->dac_enabled)
                chan->enabled = false;
            break;
        }

        case NR13_REGISTER:
        case NR23_REGISTER:
        {
            apu_pulse_channel *chan;
            if (address == NR13_REGISTER)
                chan = &apu->channel_one;
            else
                chan = &apu->channel_two;

            chan->wavelength = (chan->wavelength & 0x0700) | value;
            break;
        }

        case NR14_REGISTER:
        case NR24_REGISTER:
        {
            apu_pulse_channel *chan;
            APU_CHANNELS channelno;
            if (address == NR14_REGISTER)
            {
                chan = &apu->channel_one;
                channelno = CHANNEL_ONE;
            }
            else
            {
                chan = &apu->channel_two;
                channelno = CHANNEL_TWO;
            }

            if ((value >> 7) & 1)
                trigger_channel(apu, channelno);
            chan->length_timer_enable = (value >> 6) & 1;
            chan->wavelength = (chan->wavelength & 0x00ff) | (value & 0x7) << 8;

            // reload timer if it's zero
            if (!chan->length_timer)
                chan->length_timer = 64;
            break;
        }

        case NR30_REGISTER:
            apu->channel_three.dac_enabled = value & 0x80;
            break;

        case NR31_REGISTER:
            apu->channel_three.length_timer = 256 - value;
            break;

        case NR32_REGISTER:
            apu->channel_three.output_level = (value >> 5) & 0x3;
            break;

        case NR33_REGISTER:
            apu->channel_three.wavelength &= 0xff00;
            apu->channel_three.wavelength |= value;
            break;

        case NR34_REGISTER:
            if (value & 0x80)
                trigger_channel(apu, CHANNEL_THREE);

            apu->channel_three.length_timer_enable = (value >> 6) & 1;

            // reload timer if it's zero
            if (!apu->channel_three.length_timer)
                apu->channel_three.length_timer = 256;

            // high three bits of 11-bit wavelenth
            apu->channel_three.wavelength &= 0x00ff;
            apu->channel_three.wavelength |= (value & 0x7) << 8;
            break;

        case NR41_REGISTER:
            apu->channel_four.length_timer = 64 - (value & 0x3f);
            break;

        case NR42_REGISTER:
            apu->channel_four.initial_volume = (value >> 4) & 0xf;
            apu->channel_four.env_incrementing = (value >> 3) & 1;
            apu->channel_four.env_period = value & 0x7;
            apu->channel_four.dac_enabled = value & 0xf8;
            if (!apu->channel_four.dac_enabled)
                apu->channel_four.enabled = false;
            break;

        case NR43_REGISTER:
            apu->channel_four.clock_shift = (value >> 4) & 0xf;
            apu->channel_four.lfsr_width_flag = (value >> 3) & 1;
            apu->channel_four.clock_div_code = value & 0x7;
            break;

        case NR44_REGISTER:
            if (value & 0x80)
                trigger_channel(apu, CHANNEL_FOUR);

            apu->channel_four.length_timer_enable = (value >> 6) & 1;

            // reload timer if it's zero
            if (!apu->channel_four.length_timer)
                apu->channel_four.length_timer = 64;
            break;

        case NR50_REGISTER:
            apu->mix_vin_left = (value >> 7) & 1;
            apu->left_volume = (value >> 4) & 0x7;
            apu->mix_vin_right = (value >> 3) & 1;
            apu->right_volume = value & 0x7;
            break;

        case NR51_REGISTER:
            apu->panning_info = value;
            break;

        case NR52_REGISTER:
            apu->enabled = (value >> 7) & 1;
            break;

        default:
            break;
    }
}

uint8_t apu_read(gameboy *gb, uint16_t address)
{
    gb_apu *apu = gb->apu;

    // read from channel 3 wave RAM
    if (address >= 0xff30 && address <= 0xff3f)
        return apu->channel_three.wave_ram[address - 0xff30];

    uint8_t value = 0xff;
    switch (address)
    {
        case NR10_REGISTER:
        {
            apu_pulse_channel *chan = &apu->channel_one;
            value = 0x80
                    | (chan->sweep_period & 0x7) << 4
                    | (chan->sweep_decrementing << 3)
                    | (chan->sweep_slope & 0x7);
            break;
        }

        case NR11_REGISTER:
        case NR21_REGISTER:
        {
            apu_pulse_channel *chan;
            if (address == NR11_REGISTER)
                chan = &apu->channel_one;
            else
                chan = &apu->channel_two;

            value = 0xff & (chan->duty_number & 0x3) << 6;
            break;
        }

        case NR12_REGISTER:
        case NR22_REGISTER:
        {
            apu_pulse_channel *chan;
            if (address == NR12_REGISTER)
                chan = &apu->channel_one;
            else
                chan = &apu->channel_two;

            value = (chan->initial_volume & 0xf) << 4
                    | chan->env_incrementing << 3
                    | (chan->env_period & 0x7);
            break;
        }

        case NR13_REGISTER:
        case NR23_REGISTER:
            // wavelength is write-only
            break;

        case NR14_REGISTER:
        case NR24_REGISTER:
        {
            apu_pulse_channel *chan;
            if (address == NR14_REGISTER)
                chan = &apu->channel_one;
            else
                chan = &apu->channel_two;

            value = 0xbf | (chan->length_timer_enable << 6);
            break;
        }

        case NR30_REGISTER:
            value = 0xff & apu->channel_three.dac_enabled << 7;
            break;

        // channel 3 length timer is write-only
        case NR31_REGISTER:
            break;

        case NR32_REGISTER:
            value = 0xff & (apu->channel_three.output_level & 0x3) << 5;
            break;

        // channel 3 wavelength low is write-only
        case NR33_REGISTER:
            break;

        case NR34_REGISTER:
            value = 0xff & apu->channel_three.length_timer_enable << 6;
            break;

        case NR41_REGISTER:
            // length timer is write-only
            break;

        case NR42_REGISTER:
            value = (apu->channel_four.initial_volume & 0xf) << 4
                    | (apu->channel_four.env_incrementing) << 3
                    | (apu->channel_four.env_period & 0x7);
            break;

        case NR43_REGISTER:
            value = (apu->channel_four.clock_shift & 0xf) << 4
                    | (apu->channel_four.lfsr_width_flag) << 3
                    | (apu->channel_four.clock_div_code & 0x7);
            break;

        case NR44_REGISTER:
            value = 0xff & (apu->channel_four.length_timer_enable << 6);
            break;

        case NR50_REGISTER:
            value = apu->mix_vin_left << 7
                    | (apu->left_volume & 0x7) << 4
                    | apu->mix_vin_right << 3
                    | (apu->right_volume & 0x7);
            break;

        case NR51_REGISTER:
            value = apu->panning_info;
            break;

        case NR52_REGISTER:
            // unimplemented channels report as turned on
            value = apu->enabled << 7
                    | 0x7 << 4 // bits 4-6 are unused
                    | apu->channel_four.enabled << 3
                    | apu->channel_three.enabled << 2
                    | apu->channel_two.enabled << 1
                    | apu->channel_one.enabled;
            break;

        default:
            break;
    }

    return value;
}

void run_apu(gameboy *gb, uint8_t num_clocks)
{
    for (; num_clocks; --num_clocks)
    {
        // we only update channel states when the APU is on
        if (gb->apu->enabled)
        {
            ++gb->apu->clock;
            tick_channels(gb->apu);

            // frame sequencer is ticked every 8192 T-cycles (512 Hz)
            if (!(gb->apu->clock & 0x1fff))
            {
                gb->apu->clock = 0;
                tick_frame_sequencer(gb->apu);
            }
        }

        // gather samples even when the APU is off because
        // we need this to throttle emulation correctly
        sample_audio(gb);
    }
}

static void init_pulse_channel(apu_pulse_channel *chan, APU_CHANNELS channelno)
{
    switch (channelno)
    {
        case CHANNEL_ONE:
        case CHANNEL_TWO:
            // sweep actually only used by channel 1
            chan->sweep_enabled = false;
            chan->sweep_period = 0;
            chan->sweep_period_timer = chan->sweep_period;
            chan->sweep_decrementing = false;
            chan->sweep_slope = 0;

            chan->duty_number = channelno == CHANNEL_ONE ? 0x2 : 0;
            chan->duty_pos = 0;

            chan->length_timer = 0x3f;

            chan->initial_volume = channelno == CHANNEL_ONE ? 0xf : 0;
            chan->current_volume = chan->initial_volume;

            chan->env_incrementing = 0;

            chan->env_period = channelno == CHANNEL_ONE ? 0x3 : 0;
            chan->env_period_timer = chan->env_period;

            chan->wavelength = 0x0700;
            chan->wavelength_timer = (2048 - chan->wavelength) * 4;

            chan->length_timer_enable = false;
            chan->enabled = false;
            chan->dac_enabled = false;
            break;

        default:
            LOG_ERROR("Tried to initialize wave or noise"
                      " channel with pulse channel initializer.\n");
            exit(1);
            break;
    }
}

static void init_wave_channel(apu_wave_channel *chan)
{
    chan->length_timer = 0xff;
    chan->length_timer_enable = false;

    chan->output_level = 0;

    chan->wavelength = 0x0700;
    // NOTE: this is a factor of 2 different
    // from calculation for channels 1 and 2
    chan->wavelength_timer = (2048 - chan->wavelength) * 2;

    chan->wave_loc = 0;
    memset(chan->wave_ram, 0, WAVE_RAM_SIZE);

    chan->enabled = false;
    chan->dac_enabled = false;
}

static void init_noise_channel(apu_noise_channel *chan)
{
    chan->length_timer = 64 - 0x3f;
    chan->length_timer_enable = false;

    chan->initial_volume = 0;
    chan->current_volume = chan->initial_volume;
    chan->env_incrementing = false;
    chan->env_period = 0;
    chan->env_period_timer = chan->env_period;

    chan->clock_shift = 0;
    chan->lfsr_width_flag = false;
    chan->lfsr = 0;
    chan->clock_div_code = 0;

    uint16_t divisor = chan->clock_div_code
                       ? chan->clock_div_code << 4
                       : 8;
    chan->wavelength_timer = divisor << chan->clock_shift;

    chan->enabled = false;
    chan->dac_enabled = false;
}

static inline uint16_t sweep_frequency(apu_pulse_channel *chan)
{
    // L_{t+1} = L_{t} +- L_{t} / 2^{sweep_slope} (L_{t+1} can never underflow)
    uint16_t increment = chan->wavelength >> chan->sweep_slope;
    uint16_t new_wavelength = chan->wavelength;
    if (chan->sweep_decrementing)
        new_wavelength -= increment;
    else
        new_wavelength += increment;

    return new_wavelength;
}

static bool sweep_overflow_check(apu_pulse_channel *chan)
{
    uint16_t new_wavelength = sweep_frequency(chan);
    // channel is disabled instead of overflowing wavelength
    if (new_wavelength > 0x07ff)
    {
        chan->enabled = false;
        return true;
    }

    return false;
}

static void trigger_channel(gb_apu *apu, APU_CHANNELS channel)
{
    switch (channel)
    {
        case CHANNEL_ONE:
        {
            apu_pulse_channel *chan = &apu->channel_one;
            if (chan->dac_enabled)
            {
                chan->enabled = true;

                // envelope
                chan->env_period_timer = chan->env_period;
                chan->current_volume = chan->initial_volume;

                // wavelength sweep
                if (chan->sweep_period)
                    chan->sweep_period_timer = chan->sweep_period;
                else // sweep_period = 0 sets internal timer to 8 for some reason
                    chan->sweep_period_timer = 8;

                chan->sweep_enabled = chan->sweep_period || chan->sweep_slope;

                // nonzero sweep slope results in wavelength overflow check
                if (chan->sweep_slope)
                    sweep_overflow_check(chan);
            }
            break;
        }

        case CHANNEL_TWO:
            if (apu->channel_two.dac_enabled)
            {
                apu->channel_two.enabled = true;
                apu->channel_two.env_period_timer = apu->channel_two.env_period;
                apu->channel_two.current_volume = apu->channel_two.initial_volume;
            }
            break;

        case CHANNEL_THREE:
            if (apu->channel_three.dac_enabled)
                apu->channel_three.enabled = true;
            break;

        case CHANNEL_FOUR:
        {
            apu_noise_channel *chan = &apu->channel_four;
            if (chan->dac_enabled)
                chan->enabled = true;

            chan->env_period_timer = chan->env_period;
            chan->current_volume = chan->initial_volume;
            chan->lfsr = 0x7fff;
            break;
        }
    }
}

// Tick the APU channel 1 wavelength sweep
static void tick_sweep(apu_pulse_channel *chan)
{
    if (chan->sweep_period_timer)
        --chan->sweep_period_timer;

    if (!chan->sweep_period_timer)
    {
        if (chan->sweep_period)
            chan->sweep_period_timer = chan->sweep_period;
        else // sweep_period = 0 sets internal timer to 8 for some reason
            chan->sweep_period_timer = 8;

        if (chan->sweep_enabled && chan->sweep_period)
        {
            // sweep slope of zero causes sweeping to have no
            // effect but wavelength overflow check still happens
            uint16_t new_wavelength = sweep_frequency(chan);
            if (!sweep_overflow_check(chan) && chan->sweep_slope)
                chan->wavelength = new_wavelength;
        }
    }
}

static void tick_channel(gb_apu *apu, APU_CHANNELS channel)
{
    switch (channel)
    {
        case CHANNEL_ONE:
        case CHANNEL_TWO:
        {
            apu_pulse_channel *chan;
            if (channel == CHANNEL_ONE)
                chan = &apu->channel_one;
            else
                chan = &apu->channel_two;

            if (!chan->wavelength_timer)
            {
                chan->wavelength_timer = (2048 - chan->wavelength) * 4;
                chan->duty_pos = (chan->duty_pos + 1) & 0x7;
            }

            --chan->wavelength_timer;
            break;
        }

        case CHANNEL_THREE:
        {
            apu_wave_channel *chan = &apu->channel_three;

            if (!chan->wavelength_timer)
            {
                // NOTE: this is a factor of 2 different
                // from calculation for channels 1 and 2
                chan->wavelength_timer = (2048 - chan->wavelength) * 2;

                // the wave RAM pointer wraps around
                // Recall: this points to a nibble, so wrap at 32
                chan->wave_loc = (chan->wave_loc + 1) & 0x1f;
            }

            --chan->wavelength_timer;
            break;
        }

        case CHANNEL_FOUR:
        {
            apu_noise_channel *chan = &apu->channel_four;

            if (!chan->wavelength_timer)
            {
                uint16_t divisor = chan->clock_div_code
                                   ? chan->clock_div_code << 4
                                   : 8;

                chan->wavelength_timer = divisor << chan->clock_shift;

                // First two bits of LFSR are XORed together,
                // LFSR is shifted left by one bit, then result
                // is stored into LFSR bit 14.If lfsr_width_flag
                // is set then this value is also stored in bit
                // 6 after shifting LFSR.
                bool bitcalc = (chan->lfsr ^ (chan->lfsr >> 1)) & 1;
                chan->lfsr = (chan->lfsr >> 1) | (bitcalc << 14);

                if (chan->lfsr_width_flag)
                {
                    chan->lfsr &= ~(1 << 6);
                    chan->lfsr |= bitcalc << 6;
                }
            }

            --chan->wavelength_timer;
            break;
        }
    }
}

static void tick_length_counter(gb_apu *apu, APU_CHANNELS channel)
{
    switch (channel)
    {
        case CHANNEL_ONE:
        case CHANNEL_TWO:
        {
            apu_pulse_channel *chan;
            if (channel == CHANNEL_ONE)
                chan = &apu->channel_one;
            else
                chan = &apu->channel_two;

            if (chan->length_timer_enable && chan->length_timer)
            {
                --chan->length_timer;

                // channel is disabled when the timer runs out
                if (!chan->length_timer)
                    chan->enabled = false;
            }
            break;
        }

        case CHANNEL_THREE:
        {
            apu_wave_channel *chan = &apu->channel_three;

            if (chan->length_timer_enable && chan->length_timer)
            {
                --chan->length_timer;

                // channel is disabled when the timer runs out
                if (!chan->length_timer)
                    chan->enabled = false;
            }
            break;
        }

        case CHANNEL_FOUR:
        {
            apu_noise_channel *chan = &apu->channel_four;

            if (chan->length_timer_enable && chan->length_timer)
            {
                --chan->length_timer;

                // channel is disabled when the timer runs out
                if (!chan->length_timer)
                    chan->enabled = false;
            }
            break;
        }
    }
}

static void tick_volume(gb_apu *apu, APU_CHANNELS channel)
{
    switch (channel)
    {
        case CHANNEL_ONE:
        case CHANNEL_TWO:
        {
            apu_pulse_channel *chan;
            if (channel == CHANNEL_ONE)
                chan = &apu->channel_one;
            else
                chan = &apu->channel_two;

            // envelope period of zero disables volume sweeping
            if (chan->env_period)
            {
                if (chan->env_period_timer)
                    --chan->env_period_timer;

                if (!chan->env_period_timer)
                {
                    chan->env_period_timer = chan->env_period;

                    // increment/decrement volume if we're not
                    // already at max/min volume
                    if (chan->current_volume < 0xf
                        && chan->env_incrementing)
                    {
                        ++chan->current_volume;
                    }
                    else if(chan->current_volume > 0
                            && !chan->env_incrementing)
                    {
                        --chan->current_volume;
                    }
                }
            }
            break;
        }

        // channel three does not support volume envelope
        case CHANNEL_THREE:
            break;

        case CHANNEL_FOUR:
        {
            apu_noise_channel *chan = &apu->channel_four;

            // envelope period of zero disables volume sweeping
            if (chan->env_period)
            {
                if (chan->env_period_timer)
                    --chan->env_period_timer;

                if (!chan->env_period_timer)
                {
                    chan->env_period_timer = chan->env_period;

                    // increment/decrement volume if we're not
                    // already at max/min volume
                    if (chan->current_volume < 0xf
                        && chan->env_incrementing)
                    {
                        ++chan->current_volume;
                    }
                    else if(chan->current_volume > 0
                            && !chan->env_incrementing)
                    {
                        --chan->current_volume;
                    }
                }
            }
            break;
        }
    }
}

static uint8_t get_volume_shift(apu_wave_channel *chan)
{
    uint8_t volume_shift;
    switch(chan->output_level & 0x3)
    {
        case 0:
            volume_shift = 4;
            break;

        case 1:
            volume_shift = 0;
            break;

        case 2:
            volume_shift = 1;
            break;

        case 3:
            volume_shift = 2;
            break;
    }

    return volume_shift;
}

// translates the given channel's volume into
// the channel's DAC output (a value between -1.0 and +1.0)
static float get_channel_amplitude(gb_apu *apu, APU_CHANNELS channel)
{
    float dac_output = 0;
    switch (channel)
    {
        case CHANNEL_ONE:
        case CHANNEL_TWO:
        {
            apu_pulse_channel *chan;
            if (channel == CHANNEL_ONE)
                chan = &apu->channel_one;
            else
                chan = &apu->channel_two;

            if (chan->dac_enabled && chan->enabled)
            {
                float dac_input = duty_cycles[8*chan->duty_number + chan->duty_pos];
                dac_input *= chan->current_volume;

                // dac_input is a value between 0 and 15, inclusive
                dac_output = (dac_input / 7.5) - 1;
            }
        }
            break;

        case CHANNEL_THREE:
        {
            apu_wave_channel *chan = &apu->channel_three;

            if (chan->dac_enabled && chan->enabled)
            {
                // get current sample nibble
                uint8_t sample = chan->wave_ram[chan->wave_loc / 2];
                uint8_t volume_shift = get_volume_shift(chan);

                if (!(chan->wave_loc & 1)) // upper nibbles read first
                    sample >>= 4;

                sample &= 0x0f;

                dac_output = ((sample >> volume_shift) / 7.5) - 1;
            }
            break;
        }

        case CHANNEL_FOUR:
        {
            apu_noise_channel *chan = &apu->channel_four;
            if (chan->dac_enabled && chan->enabled)
            {
                float dac_input = ~chan->lfsr & 1;
                dac_input *= chan->current_volume;

                // dac_input is a value between 0 and 15, inclusive
                dac_output = (dac_input / 7.5) - 1;
            }
            break;
        }
    }

    return dac_output;
}

static inline void tick_length_counters(gb_apu *apu)
{
    tick_length_counter(apu, CHANNEL_ONE);
    tick_length_counter(apu, CHANNEL_TWO);
    tick_length_counter(apu, CHANNEL_THREE);
    tick_length_counter(apu, CHANNEL_FOUR);
}

static inline void tick_channels(gb_apu *apu)
{
    tick_channel(apu, CHANNEL_ONE);
    tick_channel(apu, CHANNEL_TWO);
    tick_channel(apu, CHANNEL_THREE);
    tick_channel(apu, CHANNEL_FOUR);
}

static inline void tick_volumes(gb_apu *apu)
{
    tick_volume(apu, CHANNEL_ONE);
    tick_volume(apu, CHANNEL_TWO);
    tick_volume(apu, CHANNEL_FOUR);
}

static inline void queue_audio(gb_apu *apu, uint8_t *data, int len)
{
    float *buff = (float *)data;
    int sample_len = len / sizeof(float);

    // we use stereo audio, so LR sample pairs
    // are pushed together for each audio frame
    for (int i = 0; i < sample_len; i += NUM_CHANNELS)
    {
        if (!apu->num_frames)
        {
            // starved buffer, fill with silence
            buff[i] = 0;
            buff[i + 1] = 0;
        }
        else
        {
            buff[i] = apu->sample_buffer[NUM_CHANNELS*apu->frame_start];
            buff[i + 1] = apu->sample_buffer[NUM_CHANNELS*apu->frame_start + 1];
            ++apu->frame_start;
            apu->frame_start %= AUDIO_BUFFER_FRAME_SIZE;
            --apu->num_frames;
        }
    }
}

static void SDLCALL audio_callback(void *userdata,
                                   SDL_AudioStream *stream,
                                   int additional_amount,
                                   int total_amount)
{
    (void)total_amount;
    gb_apu *apu = userdata;

    if (additional_amount > 0)
    {
        uint8_t *data = SDL_stack_alloc(uint8_t, additional_amount);
        if (data)
        {
            queue_audio(apu, data, additional_amount);
            SDL_PutAudioStreamData(stream, data, additional_amount);
            SDL_stack_free(data);
        }
    }
}

/* The frame sequencer ticks other components
 * according to the following table:
 *
 * Step   Length Ctr  Vol Env     Sweep
 * ---------------------------------------
 * 0      Clock       -           -
 * 1      -           -           -
 * 2      Clock       -           Clock
 * 3      -           -           -
 * 4      Clock       -           -
 * 5      -           -           -
 * 6      Clock       -           Clock
 * 7      -           Clock       -
 * ---------------------------------------
 * Rate   256 Hz      64 Hz       128 Hz
 *
 * Table source: https://nightshade256.github.io/2021/03/27/gb-sound-emulation.html
 */
static void tick_frame_sequencer(gb_apu *apu)
{
    switch(apu->frame_seq_pos)
    {
        case 0:
            tick_length_counters(apu);
            break;

        case 2:
            tick_length_counters(apu);
            tick_sweep(&apu->channel_one);
            break;

        case 4:
            tick_length_counters(apu);
            break;

        case 6:
            tick_length_counters(apu);
            tick_sweep(&apu->channel_one);
            break;

        case 7:
            tick_volumes(apu);
            break;

        default:
            break;
    }

    apu->frame_seq_pos = (apu->frame_seq_pos + 1) & 0x7;
}

// Push an LR stereo sample frame to the internal audio buffer
static void push_audio_frame(gameboy *gb)
{
    gb_apu *apu = gb->apu;
    float left_amplitude = 0, right_amplitude = 0;

    if (apu->enabled)
    {
        /************ left output channel panning ************/
        if (apu->panning_info & 0x10)
            left_amplitude += apu->curr_channel_samples[CHANNEL_ONE];

        if (apu->panning_info & 0x20)
            left_amplitude += apu->curr_channel_samples[CHANNEL_TWO];

        if (apu->panning_info & 0x40)
            left_amplitude += apu->curr_channel_samples[CHANNEL_THREE];

        if (apu->panning_info & 0x80)
            left_amplitude += apu->curr_channel_samples[CHANNEL_FOUR];

        /************ right output channel panning ************/
        if (apu->panning_info & 0x01)
            right_amplitude += apu->curr_channel_samples[CHANNEL_ONE];

        if (apu->panning_info & 0x02)
            right_amplitude += apu->curr_channel_samples[CHANNEL_TWO];

        if (apu->panning_info & 0x04)
            right_amplitude += apu->curr_channel_samples[CHANNEL_THREE];

        if (apu->panning_info & 0x08)
            right_amplitude += apu->curr_channel_samples[CHANNEL_FOUR];
    }

    /* Final output is average of all four channels
     * scaled by normalized stereo channel volume.
     * NOTE: a stereo channel volume of 0 is treated as
     * a volume of 1/8 (i.e., very quiet) and a value of
     * 7 is treated as a volume of 8/8 (no reduction).
     * The stereo channels don't mute non-silent samples.
     */
    float left_sample  = ((1 + apu->left_volume) / 8.0) * left_amplitude / 4,
          right_sample = ((1 + apu->right_volume) / 8.0) * right_amplitude / 4;

    // APU samples are scaled by the Game Boy's volume
    // slider and by our base volume scaledown factor
    left_sample  *= BASE_VOLUME_SCALEDOWN_FACTOR * gb->volume_slider / 100.;
    right_sample *= BASE_VOLUME_SCALEDOWN_FACTOR * gb->volume_slider / 100.;

    SDL_LockAudioStream(apu->audio_stream);

    // drop samples when the audio buffer is full
    // (only needed when the FPS limiter is off)
    if (apu->num_frames < AUDIO_BUFFER_FRAME_SIZE)
    {
        apu->sample_buffer[NUM_CHANNELS*apu->frame_end] = left_sample;
        apu->sample_buffer[NUM_CHANNELS*apu->frame_end + 1] = right_sample;
        ++apu->frame_end;
        apu->frame_end %= AUDIO_BUFFER_FRAME_SIZE;
        ++apu->num_frames;
    }

    // audio buffer is full, signal to throttle emulation
    gb->audio_sync_signal = apu->num_frames == AUDIO_BUFFER_FRAME_SIZE;

    SDL_UnlockAudioStream(apu->audio_stream);
}

// Single-pole infinite impulse response low-pass filter.
// See: https://www.embeddedrelated.com/showarticle/779.php
static inline float low_pass_filter(float in, float prev_out)
{
    return prev_out + LOW_PASS_FILTER_CONST * (in - prev_out);
}

// sample at APU native rate so we can apply a low-pass filter
// before downsampling to the audio device native rate
static void sample_audio(gameboy *gb)
{
    gb_apu *apu = gb->apu;
    --apu->sample_timer;

    // we write silence to the audio buffer when the APU is off
    float amplitudes[4] = {0};
    if (gb->apu->enabled)
    {
        for (uint8_t i = 0; i < 4; ++i)
            amplitudes[i] = get_channel_amplitude(apu, i);
    }

    float in, prev_out;
    for (uint8_t i = 0; i < 4; ++i)
    {
        in = amplitudes[i];
        prev_out = apu->curr_channel_samples[i];
        apu->curr_channel_samples[i] = low_pass_filter(in, prev_out);
    }

    if (!apu->sample_timer)
    {
        apu->sample_timer = T_CYCLES_PER_SAMPLE;
        push_audio_frame(gb);
    }
}
