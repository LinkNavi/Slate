#pragma once
#include "../theme/theme.h"
#include "imgui.h"
#include <string>
#include <vector>

// ----------------------------------------------------------------
// SlateUI — thin helpers over ImGui
// Enforces Slate's rounded, minimal design system.
// ----------------------------------------------------------------

namespace SlateUI {

    void apply_theme(const Theme& t);
    void update_colors(const Theme& t);

    // Theme accessors (defined in slate_ui.cpp)
    const Color& get_accent();
    const Theme& get_theme();

    inline ImVec4 to_imvec(const Color& c) { return {c.r, c.g, c.b, c.a}; }

    // ---- Layout ----
    void begin_fullscreen_window(const char* id);
    bool begin_panel(const char* id, ImVec2 pos, ImVec2 size, bool border = false);
    void end_panel();
    bool begin_card(const char* id, ImVec2 size, float rounding = 10.f);
    void end_card();

    // ---- Widgets ----
    bool button(const char* label, ImVec2 size = {0,0});
    bool button_outline(const char* label, ImVec2 size = {0,0});
    bool button_icon(const char* icon_text, const char* tooltip = nullptr);
    bool input_text(const char* label, char* buf, size_t buf_size,
                    ImGuiInputTextFlags flags = 0);
    void label_section(const char* text);
    void dot(const Color& color, float radius = 5.f);
    void separator();
    bool list_item(const char* label, bool selected = false,
                   const char* right_text = nullptr);
    void breadcrumb(const std::vector<std::string>& parts);

    // ---- Homepage widgets ----
    bool recent_project_card(const char* name, const char* path,
                              const char* last_opened);
    bool action_card(const char* icon, const char* title,
                     const char* subtitle, ImVec2 size = {180, 100});

} // namespace SlateUI