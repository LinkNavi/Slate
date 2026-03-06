#include "settings.h"
#include <fstream>
#include <cstring>

#if defined(_WIN32)
#include <windows.h>
#include <shlobj.h>
#endif

std::string AppSettings::default_path() {
#if defined(_WIN32)
    char buf[MAX_PATH] = {};
    SHGetFolderPathA(nullptr, CSIDL_APPDATA, nullptr, 0, buf);
    return std::string(buf) + "\\Slate\\settings.ini";
#else
    const char* home = getenv("HOME");
    return std::string(home ? home : ".") + "/.config/slate/settings.ini";
#endif
}

void AppSettings::save(const std::string& path) const {
    // Simple INI write — expand later
    std::ofstream f(path);
    if (!f) return;
    f << "[editor]\n";
    f << "tab_size=" << editor.tab_size << "\n";
    f << "use_spaces=" << editor.use_spaces << "\n";
    f << "word_wrap=" << editor.word_wrap << "\n";
    f << "show_line_numbers=" << editor.show_line_numbers << "\n";
    f << "auto_save=" << editor.auto_save << "\n";
    f << "auto_save_ms=" << editor.auto_save_ms << "\n";
    f << "[ai]\n";
    f << "api_key=" << ai.api_key << "\n";
    f << "inline_complete=" << ai.inline_complete << "\n";
    f << "trigger_delay_ms=" << ai.trigger_delay_ms << "\n";
    f << "[theme]\n";
    f << "mode=" << (int)theme.mode << "\n";
    f << "font_size=" << (int)theme.font_size << "\n";
    f << "use_wallpaper=" << theme.use_wallpaper << "\n";
}

AppSettings AppSettings::load(const std::string& path) {
    AppSettings s;
    std::ifstream f(path);
    if (!f) return s;
    std::string line, section;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == ';') continue;
        if (line[0] == '[') { section = line.substr(1, line.find(']') - 1); continue; }
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string k = line.substr(0, eq), v = line.substr(eq + 1);
        if (section == "editor") {
            if (k == "tab_size")          s.editor.tab_size          = std::stoi(v);
            if (k == "use_spaces")        s.editor.use_spaces        = v == "1";
            if (k == "word_wrap")         s.editor.word_wrap         = v == "1";
            if (k == "show_line_numbers") s.editor.show_line_numbers = v == "1";
            if (k == "auto_save")         s.editor.auto_save         = v == "1";
            if (k == "auto_save_ms")      s.editor.auto_save_ms      = std::stoi(v);
        } else if (section == "ai") {
            if (k == "api_key")           strncpy(s.ai.api_key, v.c_str(), sizeof(s.ai.api_key) - 1);
            if (k == "inline_complete")   s.ai.inline_complete       = v == "1";
            if (k == "trigger_delay_ms")  s.ai.trigger_delay_ms      = std::stoi(v);
        } else if (section == "theme") {
            if (k == "mode")              s.theme.mode               = (ThemeMode)std::stoi(v);
            if (k == "font_size")         s.theme.font_size          = (FontSize)std::stoi(v);
            if (k == "use_wallpaper")     s.theme.use_wallpaper      = v == "1";
        }
    }
    return s;
}