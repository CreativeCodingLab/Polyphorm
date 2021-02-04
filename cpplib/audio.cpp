#define _CRT_SECURE_NO_WARNINGS
#include "audio.h"
#include <Xaudio2.h>
#include "stb_vorbis.c"

#ifdef DEBUG
#include<stdio.h>
#define PRINT_DEBUG(message, ...) {printf("ERROR in file %s on line %d: ", __FILE__, __LINE__); printf(message, __VA_ARGS__); printf("\n");}
#else
#define PRINT_DEBUG(message, ...)
#endif

static AudioContext audio_context_;
static AudioContext *audio_context = &audio_context_;

// NOTE: Audio module releasing facilities aren't tested, so there may be some issues here
bool audio::init()
{
    CoInitialize(NULL);

    HRESULT hr = XAudio2Create(&audio_context->engine, 0, XAUDIO2_DEFAULT_PROCESSOR);
    if (FAILED(hr)) {
        PRINT_DEBUG("Failed to create XAudio2 engine.");
        return false;
    }
    hr = audio_context->engine->CreateMasteringVoice(&audio_context->master_voice);
    if (FAILED(hr)) {
        PRINT_DEBUG("Failed to create IXAudio2MasteringVoice.");
        return false;
    }

#ifdef DEBUG
    XAUDIO2_DEBUG_CONFIGURATION debug_config = {};
    debug_config.TraceMask = XAUDIO2_LOG_ERRORS | XAUDIO2_LOG_WARNINGS | XAUDIO2_LOG_MEMORY;
    audio_context->engine->SetDebugConfiguration(&debug_config, NULL);
#endif

    return true;
}

Sound audio::get_sound_ogg(void *data, uint32_t data_size)
{
    Sound sound = {};
    sound.sample_count = stb_vorbis_decode_memory((uint8_t *)data, data_size, &sound.channels, &sound.sample_rate, &sound.samples);

    WAVEFORMATEX wfx = {};
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = sound.channels;
    wfx.nSamplesPerSec = sound.sample_rate;
    wfx.nBlockAlign = 4;
    wfx.wBitsPerSample = 16;
    wfx.nAvgBytesPerSec = sound.sample_rate * 4;

    XAUDIO2_BUFFER buffer = {};
    buffer.AudioBytes = sound.sample_count * sound.channels * 2;
    buffer.pAudioData = (uint8_t *)sound.samples;
    buffer.PlayBegin = 0;
    buffer.PlayLength = sound.sample_count;
    buffer.LoopBegin = 0;
    buffer.LoopLength = sound.sample_count;
    buffer.LoopCount = XAUDIO2_LOOP_INFINITE;

    HRESULT hr = audio_context->engine->CreateSourceVoice(&sound.source_voice, &wfx);
    if(FAILED(hr)) {
        PRINT_DEBUG("Failed to create IXAudio2SourceVoice.");
        return Sound{};
    }

    hr = sound.source_voice->SubmitSourceBuffer(&buffer);
    if(FAILED(hr)) {
        sound.source_voice->DestroyVoice();
        PRINT_DEBUG("Failed to submit a source buffer.");
        return Sound{};
    }

    return sound;
}

bool audio::play_sound(Sound *sound)
{
    HRESULT hr = sound->source_voice->Start(0);
    if(FAILED(hr)) {
        PRINT_DEBUG("Failed to start a source voice.");
        return false;
    }
    return true;
}

float audio::get_playback_position(Sound *sound)
{
    XAUDIO2_VOICE_STATE voice_state;
    sound->source_voice->GetState(&voice_state, 0);
    double samples_played = (double)voice_state.SamplesPlayed;
    double total_samples = (double)sound->sample_count;
    double duration = (double)sound->sample_count / (double)sound->sample_rate;
    double position = duration * samples_played / total_samples;
    return (float)position;
}

void audio::release(Sound *sound)
{
    sound->source_voice->DestroyVoice();
    free(sound->samples);
    *sound = {};
}