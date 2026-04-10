#pragma once
#include "screen_manager.h"
#include "scripting.h"
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <functional>
#include <unordered_map>
#include <vector>
#include <regex>
#include <chrono>

enum EditorMode { NORMAL=0, EDITING=1, VISUAL=2, COMMAND=3, SEARCH=4 };
using ModeEvent = std::function<void(EditorMode prev, EditorMode next)>;

struct Editor {
    EditorMode  mode        = EditorMode::NORMAL;
    std::string command_buf;
    std::string search_buf;
    std::string status_msg;
    // scroll_offset now lives in SplitNode per-pane
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

struct HighlightRule {
    std::string type_str;
    std::regex  compiled;
};

struct WrenStatusSeg { WrenCallback cb; };

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

    void bind_wren_command(const std::string& name, WrenHandle* r, WrenHandle* m);
    void bind_wren_key(const std::string& key, WrenHandle* r, WrenHandle* m);
    void close_all();
    std::string current_buffer_name();
    std::string get_line(int n);

    int         line_count();
    void        set_line(int n, const std::string& s);
    void        insert_line(int n, const std::string& s);
    void        delete_line(int n);
    std::string get_cursor();
    void        set_cursor(int row, int col);
    std::string get_yank_reg()                     { return yank_reg_; }
    void        set_yank_reg(const std::string& s) { yank_reg_ = s; }
    std::string get_mode_str();
    void        set_mode_str(const std::string& s);
    std::string get_search_query()                 { return search_query_; }
    void        set_search_query(const std::string& s);
    bool        is_modified();
    std::string file_path();
    void        do_undo();
    void        do_redo();
    void        do_push_undo();
    std::string exec_cmd(const std::string& cmd);
    void        save_file();
    void        close_buffer();
    void        next_buffer();
    void        prev_buffer();
    void        new_buffer_named(const std::string& name);

    void bind_wren_insert_key(const std::string& key, WrenHandle* r, WrenHandle* m);
    void bind_wren_on_change(WrenHandle* r, WrenHandle* m);
    void bind_wren_on_save(WrenHandle* r, WrenHandle* m);
    void bind_wren_on_open(WrenHandle* r, WrenHandle* m);
    void bind_wren_on_mode_change(WrenHandle* r, WrenHandle* m);
    void add_wren_status_seg(WrenHandle* r, WrenHandle* m);
    void set_overlay(const std::string& text);

    void add_highlight_rule(const std::string& ext,
                            const std::string& pattern,
                            const std::string& token_type);
    void clear_highlight_rules(const std::string& ext);

private:
    void init_keybinds();
    void init_commands();
    void init_default_highlight_rules();
    void setup_buffer_hooks(Buffer& buf);

    // Returns current focused leaf's buffer (never null after init)
    Buffer& active_buf();
    int&    active_scroll();

    ftxui::Element render_pane(SplitNode& pane, int w, int h, bool focused);
    ftxui::Element render_split_tree(SplitNode& node, int w, int h);
    ftxui::Component build_editor();
    ftxui::Component build_bufferlist();
    ftxui::Component build_root();

   ftxui::Element render_line(const Buffer& buf, int row, const std::string& ext);
    static ftxui::Color token_color(const std::string& type);
    static std::string  file_ext(const std::string& path);
    ftxui::Element      make_overlay_elem();

    void do_search(const std::string& query);
    void jump_next_match(Buffer& buf, int dir);
    bool handle_pending(const std::string& key, Buffer& buf, Editor& ed);

    // ── State ────────────────────────────────────────────────────────────────
    std::unique_ptr<ScriptingEngine> scripting_;

    std::vector<RenderHook>  status_hooks_;
    std::vector<RenderHook>  overlay_hooks_;
    std::unordered_map<std::string, KeyHandler>     normal_keys_;
    std::unordered_map<std::string, KeyHandler>     insert_keys_;
    std::unordered_map<std::string, CommandHandler> commands_;

    Editor        editor;
    ScreenManager sm_;
    ftxui::ScreenInteractive screen_ = ftxui::ScreenInteractive::Fullscreen();

    std::string yank_reg_;

    int visual_anchor_row_ = 0;
    int visual_anchor_col_ = 0;

    std::string search_query_;
    struct SearchMatch { int row, col, len; };
    std::vector<SearchMatch> search_matches_;
    int search_match_idx_ = -1;

    std::string pending_key_;
    std::chrono::steady_clock::time_point pending_key_time_;
    static constexpr int PENDING_TIMEOUT_MS = 500;

    // Ctrl+W prefix pending
    bool ctrl_w_pending_ = false;

    std::string overlay_text_;
    bool        overlay_active_ = false;

    int bufferlist_cursor_ = 0;

    std::unordered_map<std::string, std::vector<HighlightRule>> highlight_rules_;

    WrenCallback wren_on_change_{};
    WrenCallback wren_on_save_{};
    WrenCallback wren_on_open_{};
    WrenCallback wren_on_mode_change_{};
    std::vector<WrenStatusSeg> wren_status_segs_;
};
