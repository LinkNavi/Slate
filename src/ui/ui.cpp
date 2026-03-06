#include "ui.h"
#include "slate_ui.h"
#include "imgui.h"

UI::UI(const Theme& theme) : m_theme(theme) {
    m_settings = AppSettings::load(AppSettings::default_path());
    m_lsp.start();

    m_homepage.callbacks.on_new_project   = [this]{ m_show_new_project = true; };
    m_homepage.callbacks.on_open_folder   = [this]{ open_folder(""); };
    m_homepage.callbacks.on_open_recent   = [this](const std::string& p){ open_folder(p); };
    m_homepage.callbacks.on_open_settings = [this]{ m_show_settings = true; };
    m_homepage.callbacks.on_clone_repo    = [](const std::string&){};

    m_homepage.set_recent({
        {"my-app",      "/home/user/projects/my-app",      "Today"},
        {"libfoo",      "/home/user/projects/libfoo",      "Yesterday"},
        {"game-engine", "/home/user/projects/game-engine", "3 days ago"},
    });
}

UI::~UI() {
    m_lsp.stop();
    m_gdb.stop();
}

void UI::update() {}

void UI::render() {
    SlateUI::begin_fullscreen_window("##root");
    ImGui::End();

    if (!m_project_open) {
        m_homepage.render();
    }

    if (m_show_settings) render_settings_dialog();
}

void UI::handle_event(const sapp_event*) {}

void UI::open_folder(const std::string& path) {
    m_project_root = path;
    m_build_dir    = path + "/build";
    m_project_open = !path.empty();
}

void UI::open_file(const std::string& path) {
    auto e = std::make_unique<Editor>();
    e->open(path);
    e->set_lsp(&m_lsp);
    e->set_theme(&m_theme);
    m_editors.push_back(std::move(e));
    m_active_editor = (int)m_editors.size() - 1;
    m_project_open  = true;
}

void UI::build_project() {
    m_meson.build(m_build_dir, [this](auto& l){ m_build_log.push_back(l); }, nullptr);
}

void UI::run_tests(const std::string& f) {
    m_meson.run_tests(m_build_dir, f, nullptr, nullptr);
}

void UI::start_debug() {}

void UI::apply_settings() {
    // Apply AI key
    m_ai.set_api_key(m_settings.ai.api_key);

    // Apply theme mode
    if (m_settings.theme.use_wallpaper)
        m_theme = Theme::from_wallpaper();
    else
        m_theme = Theme::fallback_dark();

    SlateUI::apply_theme(m_theme);
    m_settings.save(AppSettings::default_path());
}

// ---- Settings dialog ----

void UI::render_settings_dialog() {
    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImVec2 win_size = {620.f, 480.f};
    ImGui::SetNextWindowPos(
        {vp->WorkPos.x + (vp->WorkSize.x - win_size.x) * 0.5f,
         vp->WorkPos.y + (vp->WorkSize.y - win_size.y) * 0.5f},
        ImGuiCond_Always);
    ImGui::SetNextWindowSize(win_size, ImGuiCond_Always);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoCollapse;
    if (!ImGui::Begin("Settings", &m_show_settings, flags)) {
        ImGui::End();
        return;
    }

    // Sidebar tabs on the left
    ImGui::BeginChild("##settings_sidebar", {140.f, 0.f}, false);
    SlateUI::label_section("SETTINGS");
    ImGui::Spacing();

    auto tab_btn = [&](const char* label, SettingsTab tab) {
        bool active = m_settings_tab == tab;
        if (SlateUI::list_item(label, active))
            m_settings_tab = tab;
    };
    tab_btn("  Editor",  SettingsTab::Editor);
    tab_btn("  AI / API", SettingsTab::AI);
    tab_btn("  Theme",   SettingsTab::Theme);
    ImGui::EndChild();

    ImGui::SameLine();

    // Vertical separator
    ImGui::PushStyleColor(ImGuiCol_ChildBg, SlateUI::to_imvec(SlateUI::get_theme().bg_input));
    ImGui::BeginChild("##settings_divider", {1.f, 0.f}, false);
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::SameLine();

    // Content panel
    ImGui::BeginChild("##settings_content", {0.f, -36.f}, false);
    ImGui::Spacing();
    switch (m_settings_tab) {
        case SettingsTab::Editor: render_settings_editor_tab(); break;
        case SettingsTab::AI:     render_settings_ai_tab();     break;
        case SettingsTab::Theme:  render_settings_theme_tab();  break;
    }
    ImGui::EndChild();

    // Bottom bar — Save / Cancel
    SlateUI::separator();
    float btn_w = 90.f;
    ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x - btn_w * 2.f - 8.f + 140.f);
    if (SlateUI::button_outline("Cancel", {btn_w, 0.f}))
        m_show_settings = false;
    ImGui::SameLine();
    if (SlateUI::button("Save", {btn_w, 0.f})) {
        apply_settings();
        m_show_settings = false;
    }

    ImGui::End();
}

void UI::render_settings_editor_tab() {
    SlateUI::label_section("EDITOR");
    ImGui::Spacing();

    ImGui::SetNextItemWidth(80.f);
    ImGui::InputInt("Tab size", &m_settings.editor.tab_size);
    m_settings.editor.tab_size = std::max(1, std::min(8, m_settings.editor.tab_size));

    ImGui::Checkbox("Use spaces instead of tabs", &m_settings.editor.use_spaces);
    ImGui::Checkbox("Word wrap",                  &m_settings.editor.word_wrap);
    ImGui::Checkbox("Show line numbers",          &m_settings.editor.show_line_numbers);

    SlateUI::separator();
    SlateUI::label_section("AUTO SAVE");
    ImGui::Spacing();
    ImGui::Checkbox("Enable auto save", &m_settings.editor.auto_save);
    if (m_settings.editor.auto_save) {
        ImGui::SetNextItemWidth(120.f);
        ImGui::InputInt("Interval (ms)", &m_settings.editor.auto_save_ms);
        m_settings.editor.auto_save_ms = std::max(1000, m_settings.editor.auto_save_ms);
    }
}

void UI::render_settings_ai_tab() {
    SlateUI::label_section("CLAUDE API");
    ImGui::Spacing();

    ImGui::TextWrapped("Enter your Anthropic API key to enable AI features like inline completion and code explanation.");
    ImGui::Spacing();

    ImGui::SetNextItemWidth(-1.f);
    // Mask the key
    ImGuiInputTextFlags key_flags = ImGuiInputTextFlags_Password;
    ImGui::InputText("##api_key", m_settings.ai.api_key,
                     sizeof(m_settings.ai.api_key), key_flags);
    ImGui::PushStyleColor(ImGuiCol_Text, SlateUI::to_imvec(SlateUI::get_theme().fg_dim));
    ImGui::TextUnformatted("Your key is stored locally and never sent anywhere except api.anthropic.com");
    ImGui::PopStyleColor();

    SlateUI::separator();
    SlateUI::label_section("COMPLETION");
    ImGui::Spacing();

    ImGui::Checkbox("Inline completion (ghost text)", &m_settings.ai.inline_complete);

    if (m_settings.ai.inline_complete) {
        ImGui::SetNextItemWidth(120.f);
        ImGui::InputInt("Trigger delay (ms)", &m_settings.ai.trigger_delay_ms);
        m_settings.ai.trigger_delay_ms = std::max(200, m_settings.ai.trigger_delay_ms);
        ImGui::PushStyleColor(ImGuiCol_Text, SlateUI::to_imvec(SlateUI::get_theme().fg_dim));
        ImGui::TextUnformatted("Idle time after typing before AI is triggered");
        ImGui::PopStyleColor();
    }

    ImGui::Checkbox("Explain selected code on hover", &m_settings.ai.explain_on_select);
}

void UI::render_settings_theme_tab() {
    SlateUI::label_section("THEME");
    ImGui::Spacing();

    bool use_wallpaper = m_settings.theme.use_wallpaper;
    if (ImGui::Checkbox("Use wallpaper colors", &use_wallpaper))
        m_settings.theme.use_wallpaper = use_wallpaper;

    ImGui::PushStyleColor(ImGuiCol_Text, SlateUI::to_imvec(SlateUI::get_theme().fg_dim));
    ImGui::TextWrapped("Extracts a color palette from your desktop wallpaper to generate a matching theme.");
    ImGui::PopStyleColor();

    SlateUI::separator();
    SlateUI::label_section("FONT SIZE");
    ImGui::Spacing();

    int fs = (int)m_settings.theme.font_size;
    bool changed = false;
    changed |= ImGui::RadioButton("Small (13px)",  &fs, 13);
    changed |= ImGui::RadioButton("Medium (15px)", &fs, 15);
    changed |= ImGui::RadioButton("Large (18px)",  &fs, 18);
    if (changed) m_settings.theme.font_size = (FontSize)fs;

    ImGui::PushStyleColor(ImGuiCol_Text, SlateUI::to_imvec(SlateUI::get_theme().fg_dim));
    ImGui::TextUnformatted("Font size change takes effect after restart.");
    ImGui::PopStyleColor();

    SlateUI::separator();

    // Preview accent color
    SlateUI::label_section("ACCENT PREVIEW");
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, SlateUI::to_imvec(SlateUI::get_theme().accent));
    ImGui::TextUnformatted("This is the current accent color");
    ImGui::PopStyleColor();
    ImGui::PushStyleColor(ImGuiCol_Text, SlateUI::to_imvec(SlateUI::get_theme().fg_dim));
    ImGui::TextUnformatted("This is the dim foreground color");
    ImGui::PopStyleColor();
    SlateUI::button("Sample button", {0.f, 0.f});
}