#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <SDL.h>
#include "cboy/gameboy.h"
#include "cboy/apu.h"
#include "cboy/log.h"
#include "cboy/mbc.h"

static void init_pulse_channel(apu_pulse_channel *chan, APU_CHANNELS channelno);
static void init_wave_channel(apu_wave_channel *chan);
static void sample_audio(gb_apu *apu);
static void tick_frame_sequencer(gb_apu *apu);
static void trigger_channel(gb_apu *apu, APU_CHANNELS channel);

static inline void queue_audio(gb_apu *apu);
static inline void tick_channels(gb_apu *apu);

gb_apu *init_apu(void)
{
    gb_apu *apu = malloc(sizeof(gb_apu));
    if (apu == NULL)
        return NULL;

    apu->enabled = false;
    apu->panning_info = 0;
    apu->left_volume = 0x7;
    apu->right_volume = 0x7;
    apu->mix_vin_left = false;
    apu->mix_vin_right = false;
    apu->sample_timer = T_CYCLES_PER_SAMPLE;
    apu->frame_seq_pos = 0;
    apu->clock = 0;

    for (uint16_t i = 0; i < AUDIO_BUFFER_SIZE; ++i)
        apu->sample_buffer[i] = 0;

    apu->num_samples = 0;

    init_pulse_channel(&apu->channel_one, CHANNEL_ONE);
    init_pulse_channel(&apu->channel_two, CHANNEL_TWO);
    init_wave_channel(&apu->channel_three);

    if (SDL_Init(SDL_INIT_AUDIO) < 0)
        goto init_error;

    SDL_AudioSpec desired_spec = {
        .freq = AUDIO_SAMPLE_RATE,
        .format = AUDIO_F32SYS,
        .channels = NUM_CHANNELS,
        .samples = AUDIO_BUFFER_SIZE / NUM_CHANNELS,
        .callback = NULL,
        .userdata = NULL
    };

    apu->audio_dev = SDL_OpenAudioDevice(NULL, 0, &desired_spec, &apu->audio_spec, 0);

    if (!apu->audio_dev)
        goto init_error;

    queue_audio(apu);
    SDL_PauseAudioDevice(apu->audio_dev, 0);

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

    if (apu->audio_dev)
        SDL_CloseAudioDevice(apu->audio_dev);

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
            chan->sweep_incrementing = (value >> 3) & 1;
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
                    | (chan->sweep_incrementing << 3)
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
                    | 0x3 << 2
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
    if (!gb->apu->enabled)
        return;

    for (; num_clocks; --num_clocks)
    {
        ++gb->apu->clock;
        --gb->apu->sample_timer;
        tick_channels(gb->apu);

        // frame sequencer is ticked every 8192 T-cycles (512 Hz)
        if (!(gb->apu->clock & 0x1fff))
        {
            gb->apu->clock = 0;
            tick_frame_sequencer(gb->apu);
        }

        // gather sample
        if (!gb->apu->sample_timer)
        {
            gb->apu->sample_timer = T_CYCLES_PER_SAMPLE;
            sample_audio(gb->apu);
        }

        // audio buffer full, queue to audio device
        if (gb->apu->num_samples == AUDIO_BUFFER_SIZE)
        {
            queue_audio(gb->apu);
            gb->apu->num_samples = 0;
        }
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
            chan->sweep_incrementing = false;
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

static inline uint16_t sweep_frequency(apu_pulse_channel *chan)
{
    // L_{t+1} = L_{t} +- L_{t} / 2^{sweep_slope} (L_{t+1} can never underflow)
    uint16_t increment = chan->wavelength >> chan->sweep_slope;
    uint16_t new_wavelength = chan->wavelength;
    if (chan->sweep_incrementing)
        new_wavelength += increment;
    else
        new_wavelength -= increment;

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
            break;
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
            break;
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
            break;
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
            break;
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
            break;
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

static inline void queue_audio(gb_apu *apu)
{
    SDL_QueueAudio(apu->audio_dev,
                   apu->sample_buffer,
                   AUDIO_BUFFER_SIZE * sizeof(float));
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

static void sample_audio(gb_apu *apu)
{
    // left and right output amplitudes
    float left_amplitude = 0, right_amplitude = 0;
    float chan1_amplitude = get_channel_amplitude(apu, CHANNEL_ONE);
    float chan2_amplitude = get_channel_amplitude(apu, CHANNEL_TWO);
    float chan3_amplitude = get_channel_amplitude(apu, CHANNEL_THREE);

    /************ left output channel panning ************/
    if (apu->panning_info & 0x10)
        left_amplitude += chan1_amplitude;

    if (apu->panning_info & 0x20)
        left_amplitude += chan2_amplitude;

    if (apu->panning_info & 0x40)
        left_amplitude += chan3_amplitude;

    /************ right output channel panning ************/
    if (apu->panning_info & 0x01)
        right_amplitude += chan1_amplitude;

    if (apu->panning_info & 0x02)
        right_amplitude += chan2_amplitude;

    if (apu->panning_info & 0x04)
        right_amplitude += chan3_amplitude;

    // final output is average of all four channels
    // scaled by normalized stereo channel volume
    apu->sample_buffer[apu->num_samples++] = (apu->left_volume / 7.0)
                                             * left_amplitude / 4;
    apu->sample_buffer[apu->num_samples++] = (apu->right_volume / 7.0)
                                             * right_amplitude / 4;
}
