#include "file_picker.hpp"
#include "SDL3/SDL_dialog.h"
#include "dusk/config.hpp"

namespace dusk::ui {
    FilePickerButton::FilePickerButton(
        Rml::Element* parent,
        Props props)
        : BaseStringButton(parent, {.key = std::move(props.key)}),
        mGetValue(std::move(props.getValue)),
        mSetValue(std::move(props.setValue)),
        mDirectoryMode(props.directoryMode)
    {}

    Rml::String FilePickerButton::format_value() {
        auto path = mGetValue();

        if (path.empty())
            return "None";

        return std::filesystem::path(path).filename().string();
    }

    void FilePickerButton::set_value(Rml::String value) {
        if (mSetValue)
            mSetValue(value);
    }

    bool FilePickerButton::handle_nav_command(NavCommand cmd) {
        if (cmd == NavCommand::Confirm) {
            open_dialog();
            return true;
        }

        return BaseStringButton::handle_nav_command(cmd);
    }

    static SDL_DialogFileFilter musicFilters[] = {
        { "Audio Files", "*.wav" },
    };

    void FilePickerButton::open_dialog() {
        auto callback =
            [](void* userdata,
            const char* const* files,
            int filter)
            {
                auto* self = static_cast<FilePickerButton*>(userdata);

                if (!files || !files[0])
                    return;

                self->mSetValue(files[0]);

                config::Save();
            };

            SDL_ShowOpenFileDialog(
                callback,
                this,
                nullptr,
                musicFilters,
                0,       // num filters
                nullptr, // default location
                false);
    }
}
