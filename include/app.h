#pragma once
#include "screen_manager.h"
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <functional>
#include <unordered_map>
#include <vector>
#include "scripting.h"
enum EditorMode {
    NORMAL = 0,
    EDITING = 1,
    VISUAL = 2,
    COMMAND = 3,
};

using ModeEvent = std::function<void(EditorMode prev, EditorMode next)>;

struct Editor {
    EditorMode mode = EditorMode::NORMAL;
    std::string command_buf;
    std::string status_msg;
    int scroll_offset = 0;
    std::vector<ModeEvent> on_mode_change;

    void set_mode(EditorMode m) {
        if (m == mode) return;
        for (auto& f : on_mode_change) f(mode, m);
        mode = m;
    }
};

using KeyHandler     = std::function<void(Buffer&, Editor&)>;
using CommandHandler = std::function<void(Buffer*, Editor&, const std::string&)>;
using RenderHook     = std::function<ftxui::Element()>;

class VedApp {
public:
    VedApp();
    void run();
void set_status(const std::string& msg) { editor.status_msg = msg; }
    void open_file(const std::string& path);
    void bind_command(const std::string& name, CommandHandler fn);
    void bind_normal_key(const std::string& key, KeyHandler fn) { normal_keys_[key] = fn; }
    void bind_insert_key(const std::string& key, KeyHandler fn) { insert_keys_[key] = fn; }
    void add_status_hook(RenderHook fn)  { status_hooks_.push_back(fn); }
    void add_overlay_hook(RenderHook fn) { overlay_hooks_.push_back(fn); }
void bind_wren_command(const std::string& name, WrenHandle* receiver, WrenHandle* method);
void bind_wren_key(const std::string& key, WrenHandle* receiver, WrenHandle* method);
void close_all();
std::string current_buffer_name();
std::string get_line(int n);
private:
    void init_keybinds();
    void init_commands();
std::unique_ptr<ScriptingEngine> scripting_;
    std::vector<RenderHook> status_hooks_;
    std::vector<RenderHook> overlay_hooks_;
    std::unordered_map<std::string, KeyHandler>     normal_keys_;
    std::unordered_map<std::string, KeyHandler>     insert_keys_;
    std::unordered_map<std::string, CommandHandler> commands_;

    Editor editor;
    ScreenManager sm_;
    ftxui::ScreenInteractive screen_ = ftxui::ScreenInteractive::Fullscreen();

    ftxui::Component build_editor();
    ftxui::Component build_root();
};
