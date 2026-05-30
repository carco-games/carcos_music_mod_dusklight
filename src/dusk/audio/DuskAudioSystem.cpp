#include "dusk/audio/DuskAudioSystem.h"

#include <SDL3/SDL_init.h>
#include <array>
#include <cassert>
#include <span>
#include <mutex>

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
    Wav(std::string name, int loop_start_pos, int freq, int channels);
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
    std::string name = "";
    int fadeOutFrames = 0;
    int fadeOutTimer = 0;
    bool pauseOnceFadedOut = false;
    bool deleteOnceFadedOut = false;
    int fadeInFrames = 0;
    int fadeInTimer = 0;
};

Wav::Wav(std::string name, int loop_start_pos, int freq, int channels) {
    sampleRate = freq;
    this->channels = channels;
    loopStart = ((loop_start_pos / 1000.0f) * sampleRate);
    this->name = name;
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
            } else if (deleteOnceFadedOut) {
                dusk::audio::DeleteWav(name);
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

std::mutex audioMutex;

std::vector<std::unique_ptr<Wav>> ActiveWavs;
std::vector<std::unique_ptr<Wav>> PendingWavs;

void dusk::audio::PlayWav(const char* path, std::string name, int loop_start_pos) {
    SDL_AudioSpec spec;
    Uint8* data = nullptr;
    u32 len = 0;

    SDL_LoadWAV(path, &spec, &data, &len);

    auto wav = std::make_unique<Wav>(name, loop_start_pos, spec.freq, spec.channels);
    if (spec.channels != 2) {
        SDL_free(data);
        return;
    }

    wav->setStep((float)wav->sampleRate / (float)SampleRate);

    int sampleCount = len / sizeof(int16_t);
    int16_t* pcm = (int16_t*)data;
    wav->samples.resize(sampleCount);
    for (int i = 0; i < sampleCount; i++) {
        wav->samples[i] = pcm[i] / 32768.0f;
    }

    SDL_free(data);

    {
        std::lock_guard<std::mutex> lock(audioMutex);
        PendingWavs.emplace_back(std::move(wav));
    }
}

void CommitAudioWavs() {
    std::lock_guard<std::mutex> lock(audioMutex);

    if (!PendingWavs.empty()) {
        ActiveWavs.insert(
            ActiveWavs.end(),
            std::make_move_iterator(PendingWavs.begin()),
            std::make_move_iterator(PendingWavs.end())
        );
        PendingWavs.clear();
    }
}

void RenderAudioSubframe() {
    ZoneScoped;
    CommitAudioWavs();

    OutBuffer = {};

    JASDriver::updateDSP();
    DspRender(OutBuffer);

    // Carco's Music Mod
    // Add WAV samples to buffer
    for (auto& wav : ActiveWavs) {
        // Handle fading out or fading in if necessary
        if (wav->targetVolume != wav->volume) {
            wav->fadeOut();
            wav->fadeIn();
        }
        
        if (wav->paused)
            continue;

        int maxFrame = wav->samples.size() / wav->channels;
        if (maxFrame <= 1)
            continue;

        float pos = wav->pos;

        for (int i = 0; i < DSP_SUBFRAME_SIZE; i++) {
            int f0 = (int)pos;
            int f1 = f0 + 1;

            if (f1 >= maxFrame) break;

            float t = pos - f0;

            int stride = wav->channels;

            int i0 = f0 * stride;
            int i1 = f1 * stride;

            float l0 = wav->samples[i0];
            float r0 = wav->samples[i0 + 1];

            float l1 = wav->samples[i1];
            float r1 = wav->samples[i1 + 1];

            OutBuffer.channels[0][i] += (l0 + (l1 - l0) * t) * wav->volume;
            OutBuffer.channels[1][i] += (r0 + (r1 - r0) * t) * wav->volume;

            pos += wav->step;
            if (pos >= maxFrame) {
                if (wav->loop) {
                    pos = wav->loopStart;
                } else {
                    wav->paused = true;
                    break;
                }
            }
        }

        wav->pos = pos;
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
    for (size_t i = 0; i < ActiveWavs.size();) {
        auto& wav = ActiveWavs[i];

        int maxFrame = wav->samples.size() / wav->channels;

        if (wav->pos >= maxFrame) {
            if (wav->loop) {
                wav->pos = wav->loopStart;
                i++;
            } else {
                ActiveWavs.erase(ActiveWavs.begin() + i);
            }
        } else {
            i++;
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
void dusk::audio::SetWavVolume(std::string name, f32 volume) {
    for (auto& wav : ActiveWavs) {
        wav->volume = volume;
    }
}

void dusk::audio::SetWavTargetVolume(std::string name, f32 targetVolume) {
    for (auto& wav : ActiveWavs) {
        if (wav->name == name) {
            wav->targetVolume = targetVolume;
        }
    }
}

void dusk::audio::PauseWav(std::string name) {
    for (auto& wav : ActiveWavs) {
        if (wav->name == name) {
            wav->paused = true;
        }
    }
}

void dusk::audio::ResumeWav(std::string name, int fadeInFrames) {
    for (auto& wav : ActiveWavs) {
        if (wav->name == name) {
            wav->paused = false;
            wav->fadeInFrames = fadeInFrames;
        }
    }
}

void dusk::audio::FadeOutToPause(std::string name, int frames) {
    for (auto& wav : ActiveWavs) {
        if (wav->name == name) {
            wav->fadeOutFrames = frames;
            wav->targetVolume = 0.0f;
            wav->pauseOnceFadedOut = true;
        }
    }
}

void dusk::audio::FadeOutToDelete(std::string name, int frames) {
    for (auto& wav : ActiveWavs) {
        if (wav->name == name) {
            wav->fadeOutFrames = frames;
            wav->targetVolume = 0.0f;
            wav->deleteOnceFadedOut = true;
        }
    }
}

void dusk::audio::FadeOutToDeleteAll(int frames) {
    for (auto& wav : ActiveWavs) {
        wav->fadeOutFrames = frames;
        wav->targetVolume = 0.0f;
        wav->deleteOnceFadedOut = true;
    }
}

void dusk::audio::FadeIn(std::string name, int frames, f32 targetVolume) {
    for (auto& wav : ActiveWavs) {
        if (wav->name == name) {
            wav->fadeInFrames = frames;
            wav->targetVolume = targetVolume;
        }
    }
}

void dusk::audio::DeleteWav(std::string name) {
    for (int i = 0; i < ActiveWavs.size(); i++) {
        if (ActiveWavs[i]->name == name) {
            ActiveWavs.erase(ActiveWavs.begin() + i);
        }
    }
}
