#include "homepage.h"
#include "slate_ui.h"
#include "imgui.h"
#include <algorithm>

void Homepage::set_recent(std::vector<RecentProject> projects) {
    m_recent = std::move(projects);
}

void Homepage::render() {
    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove     | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.f, 0.f});
    ImGui::Begin("##homepage", nullptr, flags);
    ImGui::PopStyleVar();

    render_titlebar();

    // Main content area — two columns
    float total_h  = vp->WorkSize.y - 48.f; // minus titlebar
    float left_w   = 320.f;
    float right_w  = vp->WorkSize.x - left_w;
    float pad      = 24.f;

    // Left: recent projects
    ImGui::SetCursorPos({0.f, 48.f});
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));
    ImGui::BeginChild("##left_col", {left_w, total_h}, false);
    ImGui::PopStyleColor();
    ImGui::SetCursorPos({pad, pad});
    ImGui::BeginGroup();
    render_recent();
    ImGui::EndGroup();
    ImGui::EndChild();

    // Right: actions + clone
    ImGui::SetCursorPos({left_w, 48.f});
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_ChildBg));
    ImGui::BeginChild("##right_col", {right_w, total_h}, false);
    ImGui::PopStyleColor();
    ImGui::SetCursorPos({pad, pad});
    ImGui::BeginGroup();
    render_actions();
    SlateUI::separator();
    render_clone_bar();
    ImGui::EndGroup();
    ImGui::EndChild();

    ImGui::End();
}

void Homepage::render_titlebar() {
    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 p = vp->WorkPos;

    // Titlebar background
    dl->AddRectFilled(p, {p.x + vp->WorkSize.x, p.y + 48.f},
        ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_ChildBg)));

    // Subtle bottom border
    dl->AddLine({p.x, p.y + 47.f}, {p.x + vp->WorkSize.x, p.y + 47.f},
        ImGui::ColorConvertFloat4ToU32({0.f, 0.f, 0.f, 0.3f}));

    ImGui::SetCursorPos({16.f, 14.f});
    ImGui::PushStyleColor(ImGuiCol_Text, SlateUI::to_imvec(SlateUI::get_accent())); // accent color for logo
    ImGui::Text("  Slate");
    ImGui::PopStyleColor();

    // Settings button top-right
    ImGui::SetCursorPos({vp->WorkSize.x - 44.f, 10.f});
    if (SlateUI::button_icon("[=]", "Settings"))
        if (callbacks.on_open_settings) callbacks.on_open_settings();
}

void Homepage::render_actions() {
    SlateUI::label_section("START");
    ImGui::Spacing();

    float card_w = 160.f;
    float card_h = 90.f;

    if (SlateUI::action_card("[+]", "New Project", "meson init", {card_w, card_h}))
        if (callbacks.on_new_project) callbacks.on_new_project();

    ImGui::SameLine(0.f, 10.f);

    if (SlateUI::action_card("[>]", "Open Folder", "existing project", {card_w, card_h}))
        if (callbacks.on_open_folder) callbacks.on_open_folder();
}

void Homepage::render_recent() {
    // Search bar
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 8.f);
    SlateUI::input_text("##search", m_search_buf, sizeof(m_search_buf));
    if (ImGui::IsItemActive() || strlen(m_search_buf) == 0) {
        if (!ImGui::IsItemActive()) {
            ImGui::GetWindowDrawList()->AddText(
                {ImGui::GetItemRectMin().x + 8.f, ImGui::GetItemRectMin().y + 5.f},
                ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled)),
                "Search recent...");
        }
    }

    ImGui::Spacing();
    SlateUI::label_section("RECENT");
    ImGui::Spacing();

    if (m_recent.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
        ImGui::TextWrapped("No recent projects. Create or open one to get started.");
        ImGui::PopStyleColor();
        return;
    }

    std::string filter(m_search_buf);
    std::transform(filter.begin(), filter.end(), filter.begin(), ::tolower);

    for (auto& p : m_recent) {
        std::string name_lower = p.name;
        std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);
        if (!filter.empty() && name_lower.find(filter) == std::string::npos)
            continue;

        if (SlateUI::recent_project_card(p.name.c_str(), p.path.c_str(), p.last_opened.c_str()))
            if (callbacks.on_open_recent) callbacks.on_open_recent(p.path);

        ImGui::Spacing();
    }
}

void Homepage::render_clone_bar() {
    SlateUI::label_section("CLONE REPOSITORY");
    ImGui::Spacing();

    float avail = ImGui::GetContentRegionAvail().x;
    ImGui::SetNextItemWidth(avail - 90.f);
    SlateUI::input_text("##clone_url", m_clone_buf, sizeof(m_clone_buf),
                         ImGuiInputTextFlags_EnterReturnsTrue);

    // Placeholder
    if (strlen(m_clone_buf) == 0 && !ImGui::IsItemActive()) {
        ImGui::GetWindowDrawList()->AddText(
            {ImGui::GetItemRectMin().x + 8.f, ImGui::GetItemRectMin().y + 5.f},
            ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled)),
            "https://github.com/...");
    }

    ImGui::SameLine();
    if (SlateUI::button("Clone", {78.f, 0.f})) {
        if (callbacks.on_clone_repo && strlen(m_clone_buf) > 0)
            callbacks.on_clone_repo(std::string(m_clone_buf));
    }
}