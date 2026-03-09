#pragma once
#include "screen_manager.h"
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>

class VedApp {
public:
    VedApp();
    void run();

private:
    ftxui::Component build_home();
    ftxui::Component build_editor();
    ftxui::Component build_buffer_list();
    ftxui::Component build_root();

    ScreenManager sm_;
    ftxui::ScreenInteractive screen_ = ftxui::ScreenInteractive::Fullscreen();
};
