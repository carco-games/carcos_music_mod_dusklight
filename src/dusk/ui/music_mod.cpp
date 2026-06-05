#include "music_mod.hpp"

#include "pane.hpp"

namespace dusk::ui {
namespace {

}  // namespace

MusicModWindow::MusicModWindow() {
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
