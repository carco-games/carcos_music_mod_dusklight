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

void addOption(Pane& leftPane, Pane& rightPane, const Rml::String& key, ConfigVar<bool>& originalAudio,
               ConfigVar<std::string>& track, ConfigVar<f32>& volume, ConfigVar<int>& loopStartMs) {
    leftPane.register_control(
        leftPane.add_select_button({
            .key = key
        }),
        rightPane, [&](Pane& pane) {
            pane.clear();
            config_bool_select(pane, originalAudio, {
                .key = "Original Audio"
            });
            pane.add_child<FilePickerButton>(FilePickerButton::Props{
                .key = "Track",
                .getValue = [&] {
                    return track.getValue();
                },
                .setValue = [&](const std::string& path) {
                    track.setValue(path);
                }
            });
            pane.add_child<NumberButton>(NumberButton::Props{
                .key = "Individual Song Volume",
                .getValue = [&] { return static_cast<int>(std::round(volume.getValue() * 500.0f)); },
                .setValue = [&](int percent) {
                    volume.setValue(percent / 500.0f);
                    dusk::audio::SetWavVolume(track.getValue(), volume.getValue());
                    config::Save();
                },
                .max = 200,
                .suffix = "%",
            });
            pane.add_child<NumberButton>(NumberButton::Props{
                .key = "Loop Start Pos (in milliseconds)",
                .getValue = [&] { return static_cast<int>(loopStartMs.getValue()); },
                .setValue = [&](int value) {
                    loopStartMs.setValue(value);
                    config::Save();
                },
                .min = 0,
                .max = 999999,
            });
        }
    );
}

void addCategoryOptions(Pane& leftPane, Pane& rightPane, Rml::String sectionKey, Rml::String keys[], dusk::UserSettings::MusicEntry* entries[]) {
    leftPane.add_section(sectionKey);
    for (size_t i = 0; i < sizeof(entries); i++) {
        addOption(leftPane, rightPane, *keys[i], entries[i]->original, entries[i]->track,
                  entries[i]->volume, entries[i]->loopStartMs);
    }
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

        // Faron Woods -----------------------------------------------------------------------
        addOption(leftPane, rightPane, Rml::String{"Faron Woods"}, getSettings().musicMod.faronWoods.original, getSettings().musicMod.faronWoods.track,
                getSettings().musicMod.faronWoods.volume, getSettings().musicMod.faronWoods.loopStartMs);
        // -----------------------------------------------------------------------------------

        // Gerudo Desert ---------------------------------------------------------------------
        addOption(leftPane, rightPane, Rml::String{"Gerudo Desert"}, getSettings().musicMod.gerudoDesert.original, getSettings().musicMod.gerudoDesert.track,
                getSettings().musicMod.gerudoDesert.volume, getSettings().musicMod.gerudoDesert.loopStartMs);
        // -----------------------------------------------------------------------------------

        // Hidden Village --------------------------------------------------------------------
        addOption(leftPane, rightPane, Rml::String{"Hidden Village"}, getSettings().musicMod.hiddenVillage.original, getSettings().musicMod.hiddenVillage.track,
                getSettings().musicMod.hiddenVillage.volume, getSettings().musicMod.hiddenVillage.loopStartMs);
        // -----------------------------------------------------------------------------------

        // Hyrule Field ----------------------------------------------------------------------
        addOption(leftPane, rightPane, Rml::String{"Hyrule Field"}, getSettings().musicMod.hyruleField.original, getSettings().musicMod.hyruleField.track,
                getSettings().musicMod.hyruleField.volume, getSettings().musicMod.hyruleField.loopStartMs);
        // -----------------------------------------------------------------------------------

        // Kakariko Village ------------------------------------------------------------------
        addOption(leftPane, rightPane, Rml::String{"Kakariko Village"}, getSettings().musicMod.kakarikoVillage.original, getSettings().musicMod.kakarikoVillage.track,
                getSettings().musicMod.kakarikoVillage.volume, getSettings().musicMod.kakarikoVillage.loopStartMs);
        // -----------------------------------------------------------------------------------

        // Lake Hylia ------------------------------------------------------------------------
        addOption(leftPane, rightPane, Rml::String{"Lake Hylia"}, getSettings().musicMod.lakeHylia.original, getSettings().musicMod.lakeHylia.track,
                getSettings().musicMod.lakeHylia.volume, getSettings().musicMod.lakeHylia.loopStartMs);
        // -----------------------------------------------------------------------------------

        // Ordon Ranch -----------------------------------------------------------------------
        addOption(leftPane, rightPane, Rml::String{"Ordon Ranch"}, getSettings().musicMod.ordonRanch.original, getSettings().musicMod.ordonRanch.track,
                getSettings().musicMod.ordonRanch.volume, getSettings().musicMod.ordonRanch.loopStartMs);
        // -----------------------------------------------------------------------------------
    });

    add_tab("Bosses", [this](Rml::Element* content) {
        auto& leftPane = add_child<Pane>(content, Pane::Type::Controlled);
        auto& rightPane = add_child<Pane>(content, Pane::Type::Uncontrolled);

        // Argarok ----------------------------------------------------------------------------
        Rml::String argorokKeys[] = {"Argorok Intro", "Argorok Phase 1", "Argorok Phase 2 Intro", "Argorok Phase 2", "Argorok Vulnerable", "Argorok Ending"};
        dusk::UserSettings::MusicEntry* argorokEntries[] = {&getSettings().musicMod.argorokIntro, &getSettings().musicMod.argorokPhase1, &getSettings().musicMod.argorokPhase2Intro,
                                                        &getSettings().musicMod.argorokPhase2, &getSettings().musicMod.argorokVulnerable, &getSettings().musicMod.argorokEnding};
        addCategoryOptions(leftPane, rightPane, "Argorok", argorokKeys, argorokEntries);
        // -----------------------------------------------------------------------------------

        // Blizzeta ---------------------------------------------------------------------------
        Rml::String blizzetaKeys[] = {"Blizzeta Intro", "Blizzeta Phase 1", "Blizzeta Phase 2", "Blizzeta Ending"};
        dusk::UserSettings::MusicEntry* blizzetaEntries[] = {&getSettings().musicMod.blizzetaIntro, &getSettings().musicMod.blizzetaPhase1,
                                                             &getSettings().musicMod.blizzetaPhase2, &getSettings().musicMod.blizzetaEnding};
        addCategoryOptions(leftPane, rightPane, "Blizzeta", blizzetaKeys, blizzetaEntries);
        // -----------------------------------------------------------------------------------
        
        // Diababa ---------------------------------------------------------------------------
        Rml::String diababaKeys[] = {"Diababa Intro", "Diababa Phase 1", "Diababa Ook Entrance", "Diababa Phase 2", "Diababa Vulnerable", "Diababa Ending"};
        dusk::UserSettings::MusicEntry* diababaEntries[] = {&getSettings().musicMod.diababaIntro, &getSettings().musicMod.diababaPhase1, &getSettings().musicMod.diababaPhase2,
                                                            &getSettings().musicMod.diababaPhaseOok, &getSettings().musicMod.diababaVulnerable, &getSettings().musicMod.diababaEnding};
        addCategoryOptions(leftPane, rightPane, "Diababa", diababaKeys, diababaEntries);
        // -----------------------------------------------------------------------------------

        // Fyrus -----------------------------------------------------------------------------
        Rml::String fyrusKeys[] = {"Fyrus Intro", "Fyrus Main Theme", "Fyrus Vulnerable", "Fyrus Ending"};
        dusk::UserSettings::MusicEntry* fyrusEntries[] = {&getSettings().musicMod.fyrusIntro, &getSettings().musicMod.fyrusMain, &getSettings().musicMod.fyrusVulnerable,
                                                          &getSettings().musicMod.fyrusEnding};
        addCategoryOptions(leftPane, rightPane, "Fyrus", fyrusKeys, fyrusEntries);
        // -----------------------------------------------------------------------------------

        // Morpheel --------------------------------------------------------------------------
        Rml::String morpheelKeys[] = {"Morpheel Intro", "Morpheel Phase 1", "Morpheel Phase 2", "Morpheel Ending"};
        dusk::UserSettings::MusicEntry* morpheelEntries[] = {&getSettings().musicMod.morpheelIntro, &getSettings().musicMod.morpheelPhase1,
                                                         &getSettings().musicMod.morpheelPhase2, &getSettings().musicMod.morpheelEnding};
        addCategoryOptions(leftPane, rightPane, "Morpheel", morpheelKeys, morpheelEntries);
        // -----------------------------------------------------------------------------------

        // Stallord --------------------------------------------------------------------------
        Rml::String stallordKeys[] = {"Stallord Intro", "Stallord Phase 1", "Stallord Phase 2 Intro", "Stallord Phase 2", "Stallorf Ending"};
        dusk::UserSettings::MusicEntry* stallordEntries[] = {&getSettings().musicMod.stallordIntro, &getSettings().musicMod.stallordPhase1, &getSettings().musicMod.stallordPhase2Intro,
                                                             &getSettings().musicMod.stallordPhase2, &getSettings().musicMod.stallordEnding};
        addCategoryOptions(leftPane, rightPane, "Stallord", stallordKeys, stallordEntries);
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
