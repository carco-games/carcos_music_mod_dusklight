#include "dusk/audio/DuskAudioSystem.h"

#include <SDL3/SDL_init.h>
#include <array>
#include <cassert>
#include <span>

#include "JSystem/JAudio2/JASAiCtrl.h"
#include "JSystem/JAudio2/JASChannel.h"
#include "JSystem/JAudio2/JASCriticalSection.h"
#include "JSystem/JAudio2/JASDSPChannel.h"
#include "JSystem/JAudio2/JASHeapCtrl.h"

#include "DuskDsp.hpp"
#include "JSystem/JAudio2/JASAudioThread.h"
#include "JSystem/JAudio2/JASDriverIF.h"
#include "tracy/Tracy.hpp"

using namespace dusk::audio;

static OutputSubframe OutBuffer;
static std::array<f32, DSP_SUBFRAME_SIZE * OutputSubframe::NUM_CHANNELS> OutInterleaveBuffer;

static SDL_AudioStream* PlaybackStream;

/**
 * SDL audiostream callback to trigger rendering of new audio data.
 */
static void SDLCALL GetNewAudio(
    void*,
    SDL_AudioStream*,
    int needed,
    int);

/**
 * Render an entire new frame of audio and output it to SDL3.
 * Note: "audio frames" are unrelated to video frames.
 * @return Amount of audio samples rendered.
 */
static int RenderNewAudioFrame();

/**
 * Render an audio subframe and output it to SDL3.
 */
static void RenderAudioSubframe();

static void InitSDL3Output() {
    SDL_Init(SDL_INIT_AUDIO);

    constexpr SDL_AudioSpec spec = {
        SDL_AUDIO_F32,
        2,
        SampleRate,
    };
    PlaybackStream = SDL_OpenAudioDeviceStream(
        SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
        &spec,
        &GetNewAudio,
        nullptr);
}

void dusk::audio::Initialize() {
    InitSDL3Output();
    DspInit();

    JASDsp::initBuffer();
    JASDSPChannel::initAll();

    JASPoolAllocObject_MultiThreaded<JASChannel>::newMemPool(0x48);

    SDL_ResumeAudioStreamDevice(PlaybackStream);
}

void dusk::audio::SetMasterVolume(const f32 value) {
    JASCriticalSection section;

    MasterVolume = value;
}

void dusk::audio::SetPaused(const bool paused) {
    if (paused) {
        SDL_PauseAudioStreamDevice(PlaybackStream);
    } else {
        SDL_ResumeAudioStreamDevice(PlaybackStream);
    }
}

void dusk::audio::SetEnableReverb(const bool value) {
    JASCriticalSection section;

    EnableReverb = value;
}

#ifdef TRACY_ENABLE
static auto FrameName = "GetNewAudio";
#endif

void SDLCALL GetNewAudio(
    void*,
    SDL_AudioStream*,
    int needed,
    int) {
    FrameMarkStart(FrameName);
    while (needed > 0) {
        const int rendered = RenderNewAudioFrame();
        needed -= rendered;
    }
    FrameMarkEnd(FrameName);
}

int RenderNewAudioFrame() {
    ZoneScoped;
    JASCriticalSection section;
    const u32 countSubframes = JASDriver::getSubFrames();

    JASAudioThread::setDSPSyncCount(countSubframes);

    for (u32 i = 0; i < countSubframes; i++) {
        RenderAudioSubframe();

        JASAudioThread::snIntCount -= 1;
    }

    return static_cast<u16>(countSubframes) * DSP_SUBFRAME_SIZE;
}

static void InterleaveOutputData(const OutputSubframe& data, std::span<f32> target) {
    assert(target.size() >= data.channels[0].size() * OutputSubframe::NUM_CHANNELS);

    size_t outPos = 0;
    for (size_t inPos = 0; inPos < data.channels[0].size(); inPos++) {
        for (size_t channelIdx = 0; channelIdx < OutputSubframe::NUM_CHANNELS; channelIdx++) {
            target[outPos++] = data.channels[channelIdx][inPos];
        }
    }
}

// Carco's Music Mod
class Wav {
public:
    Wav(char* name, int loop_start_pos, int freq, int channels);
    void setStep(float value) { step = value; }
    void fadeOut();
    void fadeIn();

    std::vector<float> samples;
    float pos = 0.0f;
    float step = 1.0f;
    int sampleRate = 0;
    int channels = 0;
    float volume = 0.2f;
    float targetVolume = 0.2f;
    bool loop = true;
    float loopStart = 0.0f;
    bool paused = false;
    char* name = "";
    int fadeOutFrames = 0;
    int fadeOutTimer = 0;
    bool pauseOnceFadedOut = false;
    int fadeInFrames = 0;
    int fadeInTimer = 0;
};

Wav::Wav(char* name, int loop_start_pos, int freq, int channels) {
    sampleRate = freq;
    this.channels = channels;
    loopStart = ((loop_start_pos / 1000.0f) * sampleRate);
    this.name = name;
}

void Wav::fadeOut() {
    if (fadeOutFrames > 0 && !paused) {
        int divisor = fadeOutFrames - fadeOutTimer;
        if (divisor > 0) {
            volume -= 1.0f / divisor;
        } else {
            volume = 0.0f;
        }

        fadeOutTimer++;

        if (volume <= 0.0f) {
            volume = 0.0f;
            fadeOutFrames = 0;
            fadeOutTimer = 0;

            if (pauseOnceFadedOut) {
                paused = true;
                // to-do: delete wav stream if not pausing after fade
            }
        }
    }
}

void Wav::fadeIn() {
    if (fadeInFrames > 0 && !paused) {
        int divisor = fadeInFrames - fadeInTimer;
        if (divisor > 0) {
            volume += targetVolume / divisor;
        } else {
            volume = targetVolume;
        }

        fadeInTimer++;

        if (volume >= targetVolume) {
            volume = targetVolume;
            fadeInFrames = 0;
            fadeInTimer = 0;
        }
    }
}

static std::vector<Wav> ActiveWavs;

void dusk::audio::PlayWav(const char* path, char* name, int loop_start_pos) {
    SDL_AudioSpec spec;
    Uint8* data = nullptr;
    u32 len = 0;

    SDL_LoadWAV(path, &spec, &data, &len);

    Wav wav = new Wav(name, loop_start_pos, spec.freq, spec.channels);
    // wav.sampleRate = spec.freq;
    // wav.channels = spec.channels;
    // wav.loopStart = ((loop_start_pos / 1000.0f) * wav.sampleRate);
    // wav.name = name;
    wav.setStep((float)wav.sampleRate / (float)SampleRate);
    // wav.step = (float)wav.sampleRate / (float)SampleRate;

    int sampleCount = len / 2;
    int16_t* pcm = (int16_t*)data;
    wav.samples.resize(sampleCount);
    for (int i = 0; i < sampleCount; i++) {
        wav.samples[i] = pcm[i] / 32768.0f;
    }

    SDL_free(data);

    ActiveWavs.push_back(std::move(wav));
}

void RenderAudioSubframe() {
    ZoneScoped;
    OutBuffer = {};

    JASDriver::updateDSP();
    DspRender(OutBuffer);

    // Carco's Music Mod
    // Add WAV samples to buffer
    for (Wav& wav : ActiveWavs) {
        // Handle fading out or fading in if necessary
        wav.fadeOut();
        wav.fadeIn();

        if (wav.paused)
            continue;

        for (int i = 0; i < DSP_SUBFRAME_SIZE; i++) {
            float frame = wav.pos;
            int f0 = (int)frame;
            int f1 = f0 + 1;

            if ((f1 * wav.channels + 1) >= wav.samples.size()) break;

            float t = frame - f0;
            int i0 = f0 * wav.channels;
            int i1 = f1 * wav.channels;
            float l0 = wav.samples[i0];
            float r0 = wav.samples[i0 + 1];

            OutBuffer.channels[0][i] += (l0 + (wav.samples[i1] - l0) * t) * wav.volume;
            OutBuffer.channels[1][i] += (r0 + (wav.samples[i1 + 1] - r0) * t) * wav.volume;
            wav.pos += wav.step;
        }
    }

    InterleaveOutputData(OutBuffer, OutInterleaveBuffer);

    if (JASDriver::extMixCallback != nullptr && JASDriver::sMixMode == MIX_MODE_INTERLEAVE) {
        static_assert(OutputSubframe::NUM_CHANNELS == 2); // This code only works with Stereo so far.
        // NOTE: In the real game, this gets called on the entire audio frame, rather than the subframe.
        // That's probably more efficient, but I didn't wanna change the code to calculate the
        // entire audio buffers at once.
        // This is only used for the movie player, and it seems to work fine with the smaller calls.
        const auto mixData = JASDriver::extMixCallback(DSP_SUBFRAME_SIZE);
        if (mixData) {
            for (int i = 0; i < OutInterleaveBuffer.size(); i++) {
                OutInterleaveBuffer[i] += static_cast<f32>(mixData[i]) / static_cast<f32>(0x7FFF);
            }
        }
    }

    SDL_PutAudioStreamData(PlaybackStream, &OutInterleaveBuffer, sizeof(OutInterleaveBuffer));

    // Carco's Music Mod
    // Clean WAV samples
    for (size_t i = 0; i < ActiveWavs.size(); i++) {
        auto& wav = ActiveWavs[i];
        int maxFrame = (wav.samples.size() / wav.channels);
        if (wav.pos >= maxFrame) {
            if (wav.loop) {
                wav.pos = fmod(wav.pos - wav.loopStart, maxFrame - wav.loopStart) + wav.loopStart;
            } else {
                ActiveWavs.erase(ActiveWavs.begin() + i);
                continue;
            }
        }
    }
}

u32 dusk::audio::GetResetCount(int channelIdx) {
    return ChannelAux[channelIdx].resetCount;
}

f32 dusk::audio::VolumeFromU16(u16 value) {
    return static_cast<f32>(value) / static_cast<f32>(JASDriver::getChannelLevel_dsp());
}

// Carco's Music Mod
void dusk::audio::SetWavVolume(f32 volume) {
    for (auto& wav : ActiveWavs) {
        wav.volume = volume;
    }
}

void dusk::audio::PauseWav(char* name) {
    for (auto& wav : ActiveWavs) {
        if (wav.name == name) {
            wav.paused = true;
        }
    }
}

void dusk::audio::ResumeWav(char* name) {
    for (auto& wav : ActiveWavs) {
        if (wav.name == name) {
            wav.paused = false;
        }
    }
}

void dusk::audio::FadeOutToPause(char* name, int frames) {
    for (auto& wav : ActiveWavs) {
        if (wav.name == name) {
            wav.fadeOutFrames = frames;
            wav.pauseOnceFadedOut = true;
        }
    }
}
