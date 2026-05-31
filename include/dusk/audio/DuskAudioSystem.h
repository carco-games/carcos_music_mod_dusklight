#pragma once

#include <cmath>
#include <dolphin/types.h>

namespace dusk::audio {

    // Converts a 0-1 volume to a linear amplitude multiplier.
    // The curve is -4 dB per 10% step: 100% = 0 dB, 90% = -4 dB, ..., 0% = -inf dB
    inline f32 MasterVolumeToLinear(f32 v) {
        if (v <= 0.0f) {
            return 0.0f;
        }
        return std::pow(10.0f, (v - 1.0f) * 2.0f);
    }

    /**
     * Initialize the audio system and start playing audio.
     */
    void Initialize();

    void SetEnableReverb(bool value);

    void SetMasterVolume(f32 value);

    void SetPaused(bool paused);

    u32 GetResetCount(int channelIdx);

    f32 VolumeFromU16(u16 value);

    void PlayWav(const char* path, int loop_start_pos, f32 volume);
    void SetWavVolume(std::string name, f32 volume);
    void SetWavTargetVolume(std::string name, f32 targetVolume);
    void PauseWav(std::string name);
    void ResumeWav(std::string name, int fadeInFrames);
    void FadeOutToPause(std::string name, int frames);
    void FadeOutToDelete(std::string name, int frames);
    void FadeOutToDeleteAll(int frames);
    void FadeIn(std::string name, int frames, f32 targetVolume);
    void DeleteWav(std::string name);
}
