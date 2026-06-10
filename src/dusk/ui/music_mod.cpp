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
    });

    add_tab("Cutscenes", [this](Rml::Element* content) {
        auto& leftPane = add_child<Pane>(content, Pane::Type::Controlled);
        auto& rightPane = add_child<Pane>(content, Pane::Type::Uncontrolled);
    });

    add_tab("Bosses", [this](Rml::Element* content) {
        auto& leftPane = add_child<Pane>(content, Pane::Type::Controlled);
        auto& rightPane = add_child<Pane>(content, Pane::Type::Uncontrolled);
    });

    add_tab("Misc", [this](Rml::Element* content) {
        auto& leftPane = add_child<Pane>(content, Pane::Type::Controlled);
        auto& rightPane = add_child<Pane>(content, Pane::Type::Uncontrolled);
    });
}

}  // namespace dusk::ui
