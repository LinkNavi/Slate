#include "slate_ui.h"

namespace SlateUI {

static Theme s_theme;

const Color& get_accent() { return s_theme.accent; }
const Theme& get_theme()  { return s_theme; }

void apply_theme(const Theme& t) {
    s_theme = t;

    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowRounding    = 10.f;
    style.ChildRounding     = 8.f;
    style.FrameRounding     = 6.f;
    style.PopupRounding     = 8.f;
    style.ScrollbarRounding = 6.f;
    style.GrabRounding      = 6.f;
    style.TabRounding       = 6.f;

    style.WindowPadding     = {12.f, 12.f};
    style.FramePadding      = {8.f,  5.f};
    style.ItemSpacing       = {8.f,  6.f};
    style.ItemInnerSpacing  = {6.f,  4.f};
    style.ScrollbarSize     = 8.f;
    style.GrabMinSize       = 6.f;
    style.WindowBorderSize  = 0.f;
    style.ChildBorderSize   = 0.f;
    style.FrameBorderSize   = 0.f;
    style.TabBorderSize     = 0.f;
    style.IndentSpacing     = 16.f;

    update_colors(t);
}

void update_colors(const Theme& t) {
    s_theme = t;
    ImVec4* c = ImGui::GetStyle().Colors;
    auto iv  = [](const Color& col) { return ImVec4{col.r, col.g, col.b, col.a}; };
    auto dim = [](ImVec4 v, float f) { return ImVec4{v.x*f, v.y*f, v.z*f, v.w}; };

    ImVec4 bg     = iv(t.bg);
    ImVec4 panel  = iv(t.bg_panel);
    ImVec4 input  = iv(t.bg_input);
    ImVec4 fg     = iv(t.fg);
    ImVec4 fg_dim = iv(t.fg_dim);
    ImVec4 accent = iv(t.accent);
    ImVec4 accent2= iv(t.accent2);

    c[ImGuiCol_WindowBg]             = bg;
    c[ImGuiCol_ChildBg]              = panel;
    c[ImGuiCol_PopupBg]              = panel;
    c[ImGuiCol_Text]                 = fg;
    c[ImGuiCol_TextDisabled]         = fg_dim;
    c[ImGuiCol_FrameBg]              = input;
    c[ImGuiCol_FrameBgHovered]       = {accent.x*.25f, accent.y*.25f, accent.z*.25f, 1.f};
    c[ImGuiCol_FrameBgActive]        = {accent.x*.35f, accent.y*.35f, accent.z*.35f, 1.f};
    c[ImGuiCol_TitleBg]              = bg;
    c[ImGuiCol_TitleBgActive]        = panel;
    c[ImGuiCol_TitleBgCollapsed]     = bg;
    c[ImGuiCol_MenuBarBg]            = bg;
    c[ImGuiCol_ScrollbarBg]          = bg;
    c[ImGuiCol_ScrollbarGrab]        = fg_dim;
    c[ImGuiCol_ScrollbarGrabHovered] = fg;
    c[ImGuiCol_ScrollbarGrabActive]  = accent;
    c[ImGuiCol_CheckMark]            = accent;
    c[ImGuiCol_SliderGrab]           = accent;
    c[ImGuiCol_SliderGrabActive]     = accent2;
    c[ImGuiCol_Button]               = {accent.x*.8f, accent.y*.8f, accent.z*.8f, 0.85f};
    c[ImGuiCol_ButtonHovered]        = accent;
    c[ImGuiCol_ButtonActive]         = accent2;
    c[ImGuiCol_Header]               = {accent.x*.25f, accent.y*.25f, accent.z*.25f, 0.6f};
    c[ImGuiCol_HeaderHovered]        = {accent.x*.35f, accent.y*.35f, accent.z*.35f, 0.8f};
    c[ImGuiCol_HeaderActive]         = accent;
    c[ImGuiCol_Separator]            = {fg_dim.x, fg_dim.y, fg_dim.z, 0.3f};
    c[ImGuiCol_SeparatorHovered]     = accent;
    c[ImGuiCol_SeparatorActive]      = accent2;
    c[ImGuiCol_ResizeGrip]           = {0,0,0,0};
    c[ImGuiCol_ResizeGripHovered]    = dim(accent, 0.5f);
    c[ImGuiCol_ResizeGripActive]     = accent;
    c[ImGuiCol_Tab]                  = panel;
    c[ImGuiCol_TabHovered]           = dim(accent, 0.4f);
    c[ImGuiCol_TabActive]            = dim(accent, 0.6f);
    c[ImGuiCol_TabUnfocused]         = bg;
    c[ImGuiCol_TabUnfocusedActive]   = panel;
    c[ImGuiCol_PlotLines]            = accent;
    c[ImGuiCol_PlotLinesHovered]     = accent2;
    c[ImGuiCol_PlotHistogram]        = accent;
    c[ImGuiCol_PlotHistogramHovered] = accent2;
    c[ImGuiCol_TableHeaderBg]        = panel;
    c[ImGuiCol_TableBorderStrong]    = {fg_dim.x,fg_dim.y,fg_dim.z,0.2f};
    c[ImGuiCol_TableBorderLight]     = {fg_dim.x,fg_dim.y,fg_dim.z,0.1f};
    c[ImGuiCol_TableRowBg]           = {0,0,0,0};
    c[ImGuiCol_TableRowBgAlt]        = {1,1,1,0.03f};
    c[ImGuiCol_NavHighlight]         = accent;
    c[ImGuiCol_NavWindowingHighlight]= accent;
    c[ImGuiCol_NavWindowingDimBg]    = {0,0,0,0.4f};
    c[ImGuiCol_ModalWindowDimBg]     = {0,0,0,0.5f};
}

void begin_fullscreen_window(const char* id) {
    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar    | ImGuiWindowFlags_NoCollapse      |
        ImGuiWindowFlags_NoResize      | ImGuiWindowFlags_NoMove          |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0,0});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
    ImGui::Begin(id, nullptr, flags);
    ImGui::PopStyleVar(2);
}

bool begin_panel(const char* id, ImVec2 pos, ImVec2 size, bool border) {
    ImGui::SetNextWindowPos(pos);
    ImGui::SetNextWindowSize(size);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, to_imvec(s_theme.bg_panel));
    bool open = ImGui::BeginChild(id, size, border);
    ImGui::PopStyleColor();
    return open;
}
void end_panel() { ImGui::EndChild(); }

bool begin_card(const char* id, ImVec2 size, float rounding) {
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, rounding);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, to_imvec(s_theme.bg_panel));
    bool open = ImGui::BeginChild(id, size, false);
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    return open;
}
void end_card() { ImGui::EndChild(); }

bool button(const char* label, ImVec2 size) {
    return ImGui::Button(label, size);
}

bool button_outline(const char* label, ImVec2 size) {
    ImGui::PushStyleColor(ImGuiCol_Button,        {0,0,0,0});
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {s_theme.accent.r, s_theme.accent.g, s_theme.accent.b, 0.15f});
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  {s_theme.accent.r, s_theme.accent.g, s_theme.accent.b, 0.3f});
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);
    bool clicked = ImGui::Button(label, size);
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);
    return clicked;
}

bool button_icon(const char* icon_text, const char* tooltip) {
    ImGui::PushStyleColor(ImGuiCol_Button,        {0,0,0,0});
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {s_theme.accent.r, s_theme.accent.g, s_theme.accent.b, 0.15f});
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  {s_theme.accent.r, s_theme.accent.g, s_theme.accent.b, 0.3f});
    bool clicked = ImGui::Button(icon_text, {28, 28});
    ImGui::PopStyleColor(3);
    if (tooltip && ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", tooltip);
    return clicked;
}

bool input_text(const char* label, char* buf, size_t buf_size, ImGuiInputTextFlags flags) {
    return ImGui::InputText(label, buf, buf_size, flags);
}

void label_section(const char* text) {
    ImGui::PushStyleColor(ImGuiCol_Text, to_imvec(s_theme.fg_dim));
    ImGui::TextUnformatted(text);
    ImGui::PopStyleColor();
}

void dot(const Color& color, float radius) {
    ImVec2 pos = ImGui::GetCursorScreenPos();
    pos.x += radius;
    pos.y += ImGui::GetTextLineHeight() * 0.5f;
    ImGui::GetWindowDrawList()->AddCircleFilled(
        pos, radius,
        ImGui::ColorConvertFloat4ToU32(to_imvec(color)));
    ImGui::Dummy({radius * 2.f + 4.f, ImGui::GetTextLineHeight()});
}

void separator() {
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
}

bool list_item(const char* label, bool selected, const char* right_text) {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    float  w   = ImGui::GetContentRegionAvail().x;
    float  h   = ImGui::GetTextLineHeight() + style.FramePadding.y * 2.f;

    bool clicked = ImGui::InvisibleButton(label, {w, h});

    ImU32 bg = 0;
    if (selected)
        bg = ImGui::ColorConvertFloat4ToU32({s_theme.accent.r, s_theme.accent.g, s_theme.accent.b, 0.2f});
    else if (ImGui::IsItemHovered())
        bg = ImGui::ColorConvertFloat4ToU32({s_theme.fg.r, s_theme.fg.g, s_theme.fg.b, 0.06f});

    if (bg)
        ImGui::GetWindowDrawList()->AddRectFilled(pos, {pos.x+w, pos.y+h}, bg, style.FrameRounding);

    ImGui::SetCursorScreenPos({pos.x + style.FramePadding.x, pos.y + style.FramePadding.y});
    ImGui::TextUnformatted(label);

    if (right_text) {
        float rw = ImGui::CalcTextSize(right_text).x;
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - rw);
        ImGui::PushStyleColor(ImGuiCol_Text, to_imvec(s_theme.fg_dim));
        ImGui::TextUnformatted(right_text);
        ImGui::PopStyleColor();
    }
    return clicked;
}

void breadcrumb(const std::vector<std::string>& parts) {
    for (size_t i = 0; i < parts.size(); ++i) {
        if (i > 0) {
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Text, to_imvec(s_theme.fg_dim));
            ImGui::TextUnformatted("  /  ");
            ImGui::PopStyleColor();
            ImGui::SameLine();
        }
        if (i + 1 == parts.size())
            ImGui::TextUnformatted(parts[i].c_str());
        else {
            ImGui::PushStyleColor(ImGuiCol_Text, to_imvec(s_theme.fg_dim));
            ImGui::TextUnformatted(parts[i].c_str());
            ImGui::PopStyleColor();
        }
    }
}

bool recent_project_card(const char* name, const char* path, const char* last_opened) {
    float w = ImGui::GetContentRegionAvail().x;
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.f);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, to_imvec(s_theme.bg_input));

    bool clicked = false;
    std::string id = std::string("##rp_") + name;
    if (ImGui::BeginChild(id.c_str(), {w, 70.f}, false)) {
        if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0))
            clicked = true;
        if (ImGui::IsWindowHovered()) {
            ImVec2 p = ImGui::GetWindowPos();
            ImVec2 s = ImGui::GetWindowSize();
            ImGui::GetWindowDrawList()->AddRectFilled(
                p, {p.x+s.x, p.y+s.y},
                ImGui::ColorConvertFloat4ToU32({s_theme.accent.r, s_theme.accent.g, s_theme.accent.b, 0.07f}),
                8.f);
        }
        ImGuiStyle& style = ImGui::GetStyle();
        ImGui::SetCursorPos({style.WindowPadding.x, style.WindowPadding.y});
        ImGui::TextUnformatted(name);
        ImGui::PushStyleColor(ImGuiCol_Text, to_imvec(s_theme.fg_dim));
        ImGui::SetCursorPosX(style.WindowPadding.x);
        ImGui::TextUnformatted(path);
        ImGui::SetCursorPosX(style.WindowPadding.x);
        ImGui::TextUnformatted(last_opened);
        ImGui::PopStyleColor();
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    return clicked;
}

bool action_card(const char* icon, const char* title, const char* subtitle, ImVec2 size) {
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 10.f);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, to_imvec(s_theme.bg_panel));

    bool clicked = false;
    std::string id = std::string("##ac_") + title;
    if (ImGui::BeginChild(id.c_str(), size, false)) {
        if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0))
            clicked = true;
        if (ImGui::IsWindowHovered()) {
            ImVec2 p = ImGui::GetWindowPos();
            ImGui::GetWindowDrawList()->AddRectFilled(
                p, {p.x+size.x, p.y+size.y},
                ImGui::ColorConvertFloat4ToU32({s_theme.accent.r, s_theme.accent.g, s_theme.accent.b, 0.1f}),
                10.f);
            ImGui::GetWindowDrawList()->AddLine(
                {p.x+10, p.y+1}, {p.x+size.x-10, p.y+1},
                ImGui::ColorConvertFloat4ToU32(to_imvec(s_theme.accent)), 2.f);
        }
        float cx = size.x * 0.5f;
        ImGui::SetCursorPos({cx - ImGui::CalcTextSize(icon).x * 0.5f, 14.f});
        ImGui::PushStyleColor(ImGuiCol_Text, to_imvec(s_theme.accent));
        ImGui::TextUnformatted(icon);
        ImGui::PopStyleColor();
        ImGui::SetCursorPosX(cx - ImGui::CalcTextSize(title).x * 0.5f);
        ImGui::TextUnformatted(title);
        ImGui::PushStyleColor(ImGuiCol_Text, to_imvec(s_theme.fg_dim));
        ImGui::SetCursorPosX(cx - ImGui::CalcTextSize(subtitle).x * 0.5f);
        ImGui::TextUnformatted(subtitle);
        ImGui::PopStyleColor();
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    return clicked;
}

} // namespace SlateUI