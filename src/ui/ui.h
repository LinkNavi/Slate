#pragma once
#include "../theme/theme.h"
#include "../editor/editor.h"
#include "homepage.h"
#include "settings.h"
#include "../lsp/lsp_client.h"
#include "../debug/gdb_client.h"
#include "../build/meson_runner.h"
#include "../ai/claude_client.h"

#include "../sokol/sokol_app.h"

#include <vector>
#include <string>
#include <memory>

enum class Panel { FileTree, Editor, Terminal, BuildOutput, Debug, AIChat, WrapDB };
enum class SettingsTab { Editor, AI, Theme };

struct UI {
    explicit UI(const Theme& theme);
    ~UI();

    void update();
    void render();
    void handle_event(const sapp_event* e);

private:
    void render_menubar();
    void render_file_tree(float x, float y, float w, float h);
    void render_editor_tabs(float x, float y, float w, float h);
    void render_terminal(float x, float y, float w, float h);
    void render_build_output(float x, float y, float w, float h);
    void render_debug_panel(float x, float y, float w, float h);
    void render_ai_panel(float x, float y, float w, float h);
    void render_status_bar();

    void render_new_project_dialog();
    void render_wrap_db_dialog();
    void render_settings_dialog();
    void render_settings_editor_tab();
    void render_settings_ai_tab();
    void render_settings_theme_tab();

    void open_folder(const std::string& path);
    void open_file(const std::string& path);
    void build_project();
    void run_tests(const std::string& filter = "");
    void start_debug();
    void apply_settings();

    Theme        m_theme;
    AppSettings  m_settings;
    Homepage     m_homepage;
    bool         m_project_open = false;

    std::vector<std::unique_ptr<Editor>> m_editors;
    int          m_active_editor = -1;

    LSPClient    m_lsp;
    GDBClient    m_gdb;
    MesonRunner  m_meson;
    AIClient     m_ai;

    std::string  m_project_root;
    std::string  m_build_dir;

    std::vector<std::string> m_build_log;
    std::vector<std::string> m_terminal_lines;

    // UI state
    bool         m_show_new_project = false;
    bool         m_show_wrap_db     = false;
    bool         m_show_settings    = false;
    bool         m_show_debug       = false;
    bool         m_show_ai          = false;
    SettingsTab  m_settings_tab     = SettingsTab::Editor;
    float        m_sidebar_w        = 220.f;
    float        m_bottom_h         = 180.f;
};