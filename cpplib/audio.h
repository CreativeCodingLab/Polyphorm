#pragma once
#include <stdint.h>

struct IXAudio2;
struct IXAudio2MasteringVoice;
struct IXAudio2SourceVoice;

struct Sound
{
    int32_t channels;
    int32_t sample_rate;
    int32_t sample_count;
    int16_t *samples;
    IXAudio2SourceVoice *source_voice;
};

struct AudioContext
{
    IXAudio2 *engine;   
    IXAudio2MasteringVoice *master_voice;
};

namespace audio
{
    bool init();
    
    Sound get_sound_ogg(void *data, uint32_t data_size);
    bool play_sound(Sound *sound);
    float get_playback_position(Sound *sound);
    void release(Sound *sound);
}