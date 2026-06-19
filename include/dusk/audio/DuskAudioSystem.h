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

    enum FadeType {
        /* 0 */ NONE,
        /* 1 */ FADE_IN,
        /* 2 */ FADE_OUT,
    };
    
    static u32 bgmID;
    static u32 subBgmID;

    void PlayWav(dusk::UserSettings::MusicEntry entry);
    void SetWavVolume(std::string name, f32 volume);
    void SetWavTargetVolume(std::string name, f32 targetVolume);
    void PauseWav(std::string name);
    void ResumeWav(dusk::UserSettings::MusicEntry entry, int fadeInFrames);
    void FadeOut(dusk::UserSettings::MusicEntry entry, int fade_frames, bool pause_on_fade = true);
    void FadeOutToDeleteAll(int frames);
    void FadeIn(std::string name, int frames, f32 targetVolume);
    void DeleteWav(std::string name);
    void setCurrentBgmID(u32 id);
    u32 getCurrentBgmID();
    void setCurrentSubBgmID(u32 id);
    u32 getCurrentSubBgmID();
}
