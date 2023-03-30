#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <SDL.h>
#include "SDL_audio.h"
#include "cboy/gameboy.h"
#include "cboy/apu.h"
#include "cboy/log.h"
#include "cboy/mbc.h"

static void init_channel_two(apu_channel_two *chan);
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

    init_channel_two(&apu->channel_two);

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
    switch (address)
    {
        case NR21_REGISTER:
            apu->channel_two.length_timer = value & 0x3f;
            apu->channel_two.duty_number = (value >> 6) & 0x3;
            break;

        case NR22_REGISTER:
            apu->channel_two.initial_volume = (value >> 4) & 0xf;
            apu->channel_two.env_incrementing = (value >> 3) & 1;
            apu->channel_two.env_period = value & 0x7;
            apu->channel_two.dac_enabled = value & 0xf8;
            if (!apu->channel_two.dac_enabled)
                apu->channel_two.enabled = false;
            break;

        case NR23_REGISTER:
            apu->channel_two.wavelength = (apu->channel_two.wavelength & 0x0700) | value;
            break;

        case NR24_REGISTER:
            if ((value >> 7) & 1)
                trigger_channel(apu, CHANNEL_TWO);
            apu->channel_two.length_timer_enable = (value >> 6) & 1;
            apu->channel_two.wavelength = (apu->channel_two.wavelength & 0x00ff)
                                    | (value & 0x7) << 8;
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

    uint8_t value = 0xff;
    switch (address)
    {
        case NR21_REGISTER:
            value = (apu->channel_two.duty_number & 0x3) << 6
                    | (apu->channel_two.length_timer & 0x3f);
            break;

        case NR22_REGISTER:
            value = (apu->channel_two.initial_volume & 0xf) << 4
                    | apu->channel_two.env_incrementing << 3
                    | (apu->channel_two.env_period & 0x7);
            break;

        case NR23_REGISTER:
            value = apu->channel_two.wavelength & 0xff;
            break;

        case NR24_REGISTER:
            value = 0xbf | (apu->channel_two.length_timer_enable << 6);
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
                    | 0x3 << 2
                    | apu->channel_two.enabled << 1
                    | 1;
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

static void init_channel_two(apu_channel_two *chan)
{
    chan->duty_number = 0;
    chan->duty_pos = 0;

    chan->length_timer = 0x3f;

    chan->initial_volume = 0;
    chan->current_volume = chan->initial_volume;

    chan->env_incrementing = 0;

    chan->env_period = 0;
    chan->env_period_timer = chan->env_period;

    chan->wavelength = 0x0700;
    chan->wavelength_timer = (2048 - chan->wavelength) * 4;

    chan->length_timer_enable = false;
    chan->enabled = false;
    chan->dac_enabled = false;
}

static void trigger_channel(gb_apu *apu, APU_CHANNELS channel)
{
    switch (channel)
    {
        case CHANNEL_ONE:
            break;

        case CHANNEL_TWO:
            if (apu->channel_two.dac_enabled)
            {
                apu->channel_two.enabled = true;
                apu->channel_two.env_period_timer = apu->channel_two.env_period;
                apu->channel_two.current_volume = apu->channel_two.initial_volume;
            }
            break;

        case CHANNEL_THREE:
            break;

        case CHANNEL_FOUR:
            break;
    }
}

static void tick_channel(gb_apu *apu, APU_CHANNELS channel)
{
    switch (channel)
    {
        case CHANNEL_ONE:
            break;

        case CHANNEL_TWO:
            if (!apu->channel_two.wavelength_timer)
            {
                apu->channel_two.wavelength_timer = (2048 - apu->channel_two.wavelength) * 4;
                apu->channel_two.duty_pos = (apu->channel_two.duty_pos + 1) & 0x7;
            }

            --apu->channel_two.wavelength_timer;
            break;

        case CHANNEL_THREE:
            break;

        case CHANNEL_FOUR:
            break;
    }
}

static void tick_length_counter(gb_apu *apu, APU_CHANNELS channel)
{
    switch (channel)
    {
        case CHANNEL_ONE:
            break;

        case CHANNEL_TWO:
            if (apu->channel_two.length_timer_enable
                && apu->channel_two.length_timer)
            {
                --apu->channel_two.length_timer;

                // channel is disabled when the timer runs out
                if (!apu->channel_two.length_timer)
                    apu->channel_two.enabled = false;
            }
            break;

        case CHANNEL_THREE:
            break;

        case CHANNEL_FOUR:
            break;
    }
}

static void tick_volume(gb_apu *apu, APU_CHANNELS channel)
{
    switch (channel)
    {
        case CHANNEL_ONE:
            break;

        case CHANNEL_TWO:
        {
            apu_channel_two *chan = &apu->channel_two;

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

        case CHANNEL_THREE:
            break;

        case CHANNEL_FOUR:
            break;
    }
}

// translates the given channel's volume into
// the channel's DAC output (a value between -1.0 and +1.0)
static float get_channel_amplitude(gb_apu *apu, APU_CHANNELS channel)
{
    float dac_output = 0;
    switch (channel)
    {
        case CHANNEL_ONE:
            break;

        case CHANNEL_TWO:
        {
            apu_channel_two *chan = &apu->channel_two;
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
            break;

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
    tick_volume(apu, CHANNEL_THREE);
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
            // TODO: channel one sweep
            break;

        case 4:
            tick_length_counters(apu);
            break;

        case 6:
            tick_length_counters(apu);
            // TODO: channel one sweep
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
    if (apu->panning_info & 0x20)
        left_amplitude += get_channel_amplitude(apu, CHANNEL_TWO);

    if (apu->panning_info & 0x02)
        right_amplitude += get_channel_amplitude(apu, CHANNEL_TWO);

    // fianl output is average of all four channels
    // scaled by normalized stereo channel volume
    apu->sample_buffer[apu->num_samples++] = (apu->left_volume / 7.0)
                                             * left_amplitude / 4;
    apu->sample_buffer[apu->num_samples++] = (apu->right_volume / 7.0)
                                             * right_amplitude / 4;
}
