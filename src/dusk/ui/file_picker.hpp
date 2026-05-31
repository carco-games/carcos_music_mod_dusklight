#pragma once

#include "string_button.hpp"

namespace dusk::ui {

class FilePickerButton : public BaseStringButton {
public:
    struct Props {
        Rml::String key;

        std::function<std::string()> getValue;
        std::function<void(const std::string&)> setValue;

        std::string filter;
        bool directoryMode = false;
    };

    FilePickerButton(Rml::Element* parent, Props props);

    protected:
        Rml::String format_value() override;
        void set_value(Rml::String value) override;
        bool handle_nav_command(NavCommand cmd) override;

    private:
        void open_dialog();

        std::function<std::string()> mGetValue;
        std::function<void(std::string)> mSetValue;

        std::function<bool()> mIsDisabled;
        std::function<bool()> mIsModified;

        bool mDirectoryMode;
};

}  // namespace dusk::ui
