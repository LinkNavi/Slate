#pragma once
#include <string>

enum class ThemeMode { Dark, Wallpaper };
enum class FontSize  { Small = 13, Medium = 15, Large = 18 };

struct EditorSettings {
    int  tab_size         = 4;
    bool use_spaces       = true;
    bool word_wrap        = false;
    bool show_line_numbers= true;
    bool show_minimap     = false;
    bool auto_save        = false;
    int  auto_save_ms     = 5000;
};

struct AISettings {
    char api_key[256]     = {};
    bool inline_complete  = true;
    bool explain_on_select= false;
    int  trigger_delay_ms = 800; // idle ms before triggering completion
};

struct ThemeSettings {
    ThemeMode mode        = ThemeMode::Dark;
    FontSize  font_size   = FontSize::Medium;
    bool      use_wallpaper = false;
};

struct AppSettings {
    EditorSettings editor;
    AISettings     ai;
    ThemeSettings  theme;

    void save(const std::string& path) const;
    static AppSettings load(const std::string& path);
    static std::string default_path(); // platform config dir
};