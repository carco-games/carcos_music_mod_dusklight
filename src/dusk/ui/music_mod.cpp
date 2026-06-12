#include "music_mod.hpp"

#include "bool_button.hpp"
#include "dusk/config.hpp"
#include "file_picker.hpp"
#include "number_button.hpp"
#include "pane.hpp"
#include "dusk/audio/DuskAudioSystem.h"

namespace dusk::ui {
namespace {
struct ConfigBoolProps {
    Rml::String key;
    Rml::String icon;
    Rml::String helpText;
    std::function<void(bool)> onChange;
    std::function<bool()> isDisabled;
};

SelectButton& config_bool_select(
    Pane& pane, ConfigVar<bool>& var, ConfigBoolProps props) {
    auto& button = pane.add_child<BoolButton>(BoolButton::Props{
        .key = std::move(props.key),
        .icon = std::move(props.icon),
        .getValue = [&var] { return var.getValue(); },
        .setValue =
            [&var, callback = std::move(props.onChange)](bool value) {
                if (value == var.getValue()) {
                    return;
                }
                var.setValue(value);
                config::Save();
                if (callback) {
                    callback(value);
                }
            },
        .isDisabled = std::move(props.isDisabled),
        .isModified = [&var] { return var.getValue() != var.getDefaultValue(); },
    });
    return button;
}

}  // namespace

MusicModWindow::MusicModWindow() {
    add_tab("Presets", [this](Rml::Element* content) {
        auto& leftPane = add_child<Pane>(content, Pane::Type::Controlled);
        auto& rightPane = add_child<Pane>(content, Pane::Type::Uncontrolled);

        leftPane.register_control(
            leftPane.add_select_button({
                .key = "Areas Preset",
                .getValue = [] {
                    const bool preset = getSettings().musicMod.areasPreset.getValue();
                    if (preset) {
                        return Rml::String("Custom");
                    }
                    return Rml::String{"Original"};
                },
                .isModified = [] {
                    const auto& preset = getSettings().musicMod.areasPreset;
                    return preset.getValue() != preset.getDefaultValue();
                },
            }),
            rightPane, [](Pane& pane) {
                pane.clear();
                pane.add_button("Set All Area Music To Original").on_pressed([] {
                    mDoAud_seStartMenu(kSoundItemChange);
                    getSettings().musicMod.areasPreset.setValue(false);
                    config::Save();
                });
                pane.add_button("Set All Area Music To Custom").on_pressed([] {
                    mDoAud_seStartMenu(kSoundItemChange);
                    getSettings().musicMod.areasPreset.setValue(true);
                    config::Save();
                });
        });

        leftPane.register_control(
            leftPane.add_select_button({
                .key = "Cutscenes Preset",
                .getValue = [] {
                    const bool preset = getSettings().musicMod.cutscenesPreset.getValue();
                    if (preset) {
                        return Rml::String("Custom");
                    }
                    return Rml::String{"Original"};
                },
                .isModified = [] {
                    const auto& preset = getSettings().musicMod.cutscenesPreset;
                    return preset.getValue() != preset.getDefaultValue();
                },
            }),
            rightPane, [](Pane& pane) {
                pane.clear();
                pane.add_button("Set All Cutscene Music To Original").on_pressed([] {
                    mDoAud_seStartMenu(kSoundItemChange);
                    getSettings().musicMod.cutscenesPreset.setValue(false);
                    config::Save();
                });
                pane.add_button("Set All Cutscene Music To Custom").on_pressed([] {
                    mDoAud_seStartMenu(kSoundItemChange);
                    getSettings().musicMod.cutscenesPreset.setValue(true);
                    config::Save();
                });
        });

        leftPane.register_control(
            leftPane.add_select_button({
                .key = "Bosses Preset",
                .getValue = [] {
                    const bool preset = getSettings().musicMod.bossesPreset.getValue();
                    if (preset) {
                        return Rml::String("Custom");
                    }
                    return Rml::String{"Original"};
                },
                .isModified = [] {
                    const auto& preset = getSettings().musicMod.bossesPreset;
                    return preset.getValue() != preset.getDefaultValue();
                },
            }),
            rightPane, [](Pane& pane) {
                pane.clear();
                pane.add_button("Set All Boss Music To Original").on_pressed([] {
                    mDoAud_seStartMenu(kSoundItemChange);
                    getSettings().musicMod.bossesPreset.setValue(false);
                    config::Save();
                });
                pane.add_button("Set All Boss Music To Custom").on_pressed([] {
                    mDoAud_seStartMenu(kSoundItemChange);
                    getSettings().musicMod.bossesPreset.setValue(true);
                    config::Save();
                });
        });

        leftPane.register_control(
            leftPane.add_select_button({
                .key = "Misc Preset",
                .getValue = [] {
                    return Rml::String{"Original"};
                },
                .isModified = [] {
                    return 1;
                },
            }),
            rightPane, [](Pane& pane) {
                pane.clear();
                pane.add_button("Set All Misc Music To Original").on_pressed([] {
                    mDoAud_seStartMenu(kSoundItemChange);
                    getSettings().musicMod.miscPreset.setValue(false);
                    config::Save();
                });
                pane.add_button("Set All Misc Music To Custom").on_pressed([] {
                    mDoAud_seStartMenu(kSoundItemChange);
                    getSettings().musicMod.miscPreset.setValue(true);
                    config::Save();
                });
        });
    });

    add_tab("Areas", [this](Rml::Element* content) {
        auto& leftPane = add_child<Pane>(content, Pane::Type::Controlled);
        auto& rightPane = add_child<Pane>(content, Pane::Type::Uncontrolled);

        auto addOption = [&](const Rml::String& key, ConfigVar<bool>& originalAudio, ConfigVar<std::string>& trackFilepath,
                                       ConfigVar<f32>& volume, ConfigVar<int>& loopStartPos) {
            leftPane.register_control(
                leftPane.add_select_button({
                    .key = key,
                }),
                rightPane, [&](Pane& pane) {
                    pane.clear();
                    config_bool_select(pane, originalAudio, {
                        .key = "Original Audio"
                    });
                    pane.add_child<FilePickerButton>(FilePickerButton::Props{
                        .key = "Track",
                        .getValue = [&] {
                            return trackFilepath.getValue();
                        },
                        .setValue = [&](const std::string& path) {
                            trackFilepath.setValue(path);
                            config::Save();
                        },
                    });
                    pane.add_child<NumberButton>(NumberButton::Props{
                        .key = "Individual Song Volume",
                        .getValue = [&] { return static_cast<int>(std::round(volume.getValue() * 500.0f)); },
                        .setValue = [&](int percent) {
                            volume.setValue(percent / 500.0f);
                            dusk::audio::SetWavVolume(trackFilepath.getValue(), volume.getValue());
                            config::Save();
                        },
                        .max = 200,
                        .suffix = "%",
                    });
                    pane.add_child<NumberButton>(NumberButton::Props{
                        .key = "Loop Start Pos (in milliseconds)",
                        .getValue = [&] { return static_cast<int>(loopStartPos.getValue()); },
                        .setValue = [&](int value) {
                            loopStartPos.setValue(value);
                            config::Save();
                        },
                        .min = 0,
                        .max = 999999,
                    });
                }
            );
        };

        // Faron Woods -----------------------------------------------------------------------
        addOption(Rml::String{"Faron Woods"}, getSettings().musicMod.faronWoods.original, getSettings().musicMod.faronWoods.track,
                getSettings().musicMod.faronWoods.volume, getSettings().musicMod.faronWoods.loopStartMs);
        // -----------------------------------------------------------------------------------

        // Gerudo Desert ---------------------------------------------------------------------
        addOption(Rml::String{"Gerudo Desert"}, getSettings().musicMod.gerudoDesert.original, getSettings().musicMod.gerudoDesert.track,
                getSettings().musicMod.gerudoDesert.volume, getSettings().musicMod.gerudoDesert.loopStartMs);
        // -----------------------------------------------------------------------------------

        // Hidden Village --------------------------------------------------------------------
        addOption(Rml::String{"Hidden Village"}, getSettings().musicMod.hiddenVillage.original, getSettings().musicMod.hiddenVillage.track,
                getSettings().musicMod.hiddenVillage.volume, getSettings().musicMod.hiddenVillage.loopStartMs);
        // -----------------------------------------------------------------------------------

        // Hyrule Field ----------------------------------------------------------------------
        addOption(Rml::String{"Hyrule Field"}, getSettings().musicMod.hyruleField.original, getSettings().musicMod.hyruleField.track,
                getSettings().musicMod.hyruleField.volume, getSettings().musicMod.hyruleField.loopStartMs);
        // -----------------------------------------------------------------------------------

        // Kakariko Village ------------------------------------------------------------------
        addOption(Rml::String{"Kakariko Village"}, getSettings().musicMod.kakarikoVillage.original, getSettings().musicMod.kakarikoVillage.track,
                getSettings().musicMod.kakarikoVillage.volume, getSettings().musicMod.kakarikoVillage.loopStartMs);
        // -----------------------------------------------------------------------------------

        // Lake Hylia ------------------------------------------------------------------------
        addOption(Rml::String{"Lake Hylia"}, getSettings().musicMod.lakeHylia.original, getSettings().musicMod.lakeHylia.track,
                getSettings().musicMod.lakeHylia.volume, getSettings().musicMod.lakeHylia.loopStartMs);
        // -----------------------------------------------------------------------------------

        // Ordon Ranch -----------------------------------------------------------------------
        addOption(Rml::String{"Ordon Ranch"}, getSettings().musicMod.ordonRanch.original, getSettings().musicMod.ordonRanch.track,
                getSettings().musicMod.ordonRanch.volume, getSettings().musicMod.ordonRanch.loopStartMs);
        // -----------------------------------------------------------------------------------
    });

    add_tab("Bosses", [this](Rml::Element* content) {
        auto& leftPane = add_child<Pane>(content, Pane::Type::Controlled);
        auto& rightPane = add_child<Pane>(content, Pane::Type::Uncontrolled);

        auto addOption = [&](const Rml::String& key, const std::vector<Rml::String>& button_keys, std::vector<ConfigVar<bool>*> originalAudios,
                                       std::vector<ConfigVar<std::string>*> trackFilepaths, std::vector<ConfigVar<f32>*> volumes,
                                       std::vector<ConfigVar<int>*> loopStartPos) {
            leftPane.register_control(
                leftPane.add_select_button({
                    .key = key,
                }),
                rightPane, [&](Pane& pane) {
                    pane.clear();
                    for (size_t i = 0; i < button_keys.size(); i++) {
                        auto* originalAudio = originalAudios[i];
                        auto* track = trackFilepaths[i];
                        auto* volume = volumes[i];
                        auto* loopStart = loopStartPos[i];

                        config_bool_select(pane, *originalAudio, {
                            .key = "Original Audio"
                        });
                        pane.add_child<FilePickerButton>(FilePickerButton::Props{
                            .key = "Track",
                            .getValue = [track] {
                                return track->getValue();
                            },
                            .setValue = [track](const std::string& path) {
                                track->setValue(path);
                                config::Save();
                            },
                        });
                        pane.add_child<NumberButton>(NumberButton::Props{
                            .key = "Individual Song Volume",
                            .getValue = [volume] { return static_cast<int>(std::round(volume->getValue() * 500.0f)); },
                            .setValue = [volume, track](int percent) {
                                volume->setValue(percent / 500.0f);
                                dusk::audio::SetWavVolume(track->getValue(), volume->getValue());
                                config::Save();
                            },
                            .max = 200,
                            .suffix = "%",
                        });
                        pane.add_child<NumberButton>(NumberButton::Props{
                            .key = "Loop Start Pos (in milliseconds)",
                            .getValue = [loopStart] { return static_cast<int>(loopStart->getValue()); },
                            .setValue = [loopStart](int value) {
                                loopStart->setValue(value);
                                config::Save();
                            },
                            .min = 0,
                            .max = 999999,
                        });
                    }
                }
            );
        };

        // Blizzeta ---------------------------------------------------------------------------
        addOption("Blizzeta", {"Blizzeta Intro", "Blizzeta Phase 1", "Blizzeta Phase 2", "Blizzeta Ending"},
                  {&getSettings().musicMod.blizzetaIntro.original, &getSettings().musicMod.blizzetaPhase1.original, &getSettings().musicMod.blizzetaPhase2.original,
                                  &getSettings().musicMod.blizzetaEnding.original},
                  {&getSettings().musicMod.blizzetaIntro.track, &getSettings().musicMod.blizzetaPhase1.track, &getSettings().musicMod.blizzetaPhase2.track,
                                  &getSettings().musicMod.blizzetaEnding.track},
                  {&getSettings().musicMod.blizzetaIntro.volume, &getSettings().musicMod.blizzetaPhase1.volume, &getSettings().musicMod.blizzetaPhase2.volume,
                                  &getSettings().musicMod.blizzetaEnding.volume},
                  {&getSettings().musicMod.blizzetaIntro.loopStartMs, &getSettings().musicMod.blizzetaPhase1.loopStartMs, &getSettings().musicMod.blizzetaPhase2.loopStartMs,
                                  &getSettings().musicMod.blizzetaEnding.loopStartMs});
        // -----------------------------------------------------------------------------------
        
        // Diababa ---------------------------------------------------------------------------
        addOption("Diababa", {"Diababa Intro", "Diababa Phase 1", "Diababa Ook Entrance", "Diababa Phase 2", "Diababa Vulnerable", "Diababa Ending"},
                  {&getSettings().musicMod.diababaIntro.original, &getSettings().musicMod.diababaPhase1.original, &getSettings().musicMod.diababaPhase2.original,
                                  &getSettings().musicMod.diababaPhaseOok.original, &getSettings().musicMod.diababaVulnerable.original, &getSettings().musicMod.diababaEnding.original},
                  {&getSettings().musicMod.diababaIntro.track, &getSettings().musicMod.diababaPhase1.track, &getSettings().musicMod.diababaPhase2.track,
                                  &getSettings().musicMod.diababaPhaseOok.track, &getSettings().musicMod.diababaVulnerable.track, &getSettings().musicMod.diababaEnding.track},
                  {&getSettings().musicMod.diababaIntro.volume, &getSettings().musicMod.diababaPhase1.volume, &getSettings().musicMod.diababaPhase2.volume,
                           &getSettings().musicMod.diababaPhaseOok.volume, &getSettings().musicMod.diababaVulnerable.volume, &getSettings().musicMod.diababaEnding.volume},
                  {&getSettings().musicMod.diababaIntro.loopStartMs, &getSettings().musicMod.diababaPhase1.loopStartMs, &getSettings().musicMod.diababaPhase2.loopStartMs,
                                &getSettings().musicMod.diababaPhaseOok.loopStartMs, &getSettings().musicMod.diababaVulnerable.loopStartMs, &getSettings().musicMod.diababaEnding.loopStartMs});
        // -----------------------------------------------------------------------------------

        // Fyrus -----------------------------------------------------------------------------
        addOption("Fyrus", {"Fyrus Intro", "Fyrus Main Theme", "Fyrus Vulnerable", "Fyrus Ending"},
                  {&getSettings().musicMod.fyrusIntro.original, &getSettings().musicMod.fyrusMain.original,
                  &getSettings().musicMod.fyrusVulnerable.original, &getSettings().musicMod.fyrusEnding.original},
                  {&getSettings().musicMod.fyrusIntro.track, &getSettings().musicMod.fyrusMain.track,
                  &getSettings().musicMod.fyrusVulnerable.track, &getSettings().musicMod.fyrusEnding.track},
                  {&getSettings().musicMod.fyrusIntro.volume, &getSettings().musicMod.fyrusMain.volume,
                  &getSettings().musicMod.fyrusVulnerable.volume, &getSettings().musicMod.fyrusEnding.volume},
                  {&getSettings().musicMod.fyrusIntro.loopStartMs, &getSettings().musicMod.fyrusMain.loopStartMs,
                  &getSettings().musicMod.fyrusVulnerable.loopStartMs, &getSettings().musicMod.fyrusEnding.loopStartMs});
        // -----------------------------------------------------------------------------------

        // Morpheel --------------------------------------------------------------------------
        addOption("Morpheel", {"Morpheel Intro", "Morpheel Phase 1", "Morpheel Phase 2", "Morpheel Ending"},
                  {&getSettings().musicMod.morpheelIntro.original, &getSettings().musicMod.morpheelPhase1.original,
                  &getSettings().musicMod.morpheelPhase2.original, &getSettings().musicMod.morpheelEnding.original},
                  {&getSettings().musicMod.morpheelIntro.track, &getSettings().musicMod.morpheelPhase1.track,
                  &getSettings().musicMod.morpheelPhase2.track, &getSettings().musicMod.morpheelEnding.track},
                  {&getSettings().musicMod.morpheelIntro.volume, &getSettings().musicMod.morpheelPhase1.volume,
                  &getSettings().musicMod.morpheelPhase2.volume, &getSettings().musicMod.morpheelEnding.volume},
                  {&getSettings().musicMod.morpheelIntro.loopStartMs, &getSettings().musicMod.morpheelPhase1.loopStartMs,
                  &getSettings().musicMod.morpheelPhase2.loopStartMs, &getSettings().musicMod.morpheelEnding.loopStartMs});
        // -----------------------------------------------------------------------------------

        // Stallord --------------------------------------------------------------------------
        addOption("Stallord", {"Stallord Intro", "Stallord Phase 1", "Stallord Phase 2 Intro", "Stallord Phase 2", "Stallord Ending"},
                  {&getSettings().musicMod.stallordIntro.original, &getSettings().musicMod.stallordPhase1.original, &getSettings().musicMod.stallordPhase2Intro.original,
                  &getSettings().musicMod.stallordPhase2.original, &getSettings().musicMod.stallordEnding.original},
                  {&getSettings().musicMod.stallordIntro.track, &getSettings().musicMod.stallordPhase1.track, &getSettings().musicMod.stallordPhase2Intro.track,
                  &getSettings().musicMod.stallordPhase2.track, &getSettings().musicMod.stallordEnding.track},
                  {&getSettings().musicMod.stallordIntro.volume, &getSettings().musicMod.stallordPhase1.volume, &getSettings().musicMod.stallordPhase2Intro.volume,
                  &getSettings().musicMod.stallordPhase2.volume, &getSettings().musicMod.stallordEnding.volume},
                  {&getSettings().musicMod.stallordIntro.loopStartMs, &getSettings().musicMod.stallordPhase1.loopStartMs, &getSettings().musicMod.stallordPhase2Intro.loopStartMs,
                  &getSettings().musicMod.stallordPhase2.loopStartMs, &getSettings().musicMod.stallordEnding.loopStartMs});
        // -----------------------------------------------------------------------------------
    });

    add_tab("Cutscenes", [this](Rml::Element* content) {
        auto& leftPane = add_child<Pane>(content, Pane::Type::Controlled);
        auto& rightPane = add_child<Pane>(content, Pane::Type::Uncontrolled);
    });

    add_tab("Misc", [this](Rml::Element* content) {
        auto& leftPane = add_child<Pane>(content, Pane::Type::Controlled);
        auto& rightPane = add_child<Pane>(content, Pane::Type::Uncontrolled);
    });
}

}  // namespace dusk::ui
