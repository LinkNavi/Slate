#include "app.h"
#include <ftxui/dom/elements.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/component_options.hpp>

using namespace ftxui;

// Helper: build Elements inline
static Elements make_elems(std::initializer_list<Element> il) {
    return Elements(il);
}

// ─── Home Screen ─────────────────────────────────────────────────────────────

Component VedApp::build_home() {
    return Renderer([this]() -> Element {
        auto logo = vbox(make_elems({
            text("  ██╗   ██╗███████╗██████╗  ") | color(Color::Cyan),
            text("  ██║   ██║██╔════╝██╔══██╗ ") | color(Color::Cyan),
            text("  ██║   ██║█████╗  ██║  ██║ ") | color(Color::Cyan),
            text("  ╚██╗ ██╔╝██╔══╝  ██║  ██║ ") | color(Color::CyanLight),
            text("   ╚████╔╝ ███████╗██████╔╝ ") | color(Color::CyanLight),
            text("    ╚═══╝  ╚══════╝╚═════╝  ") | color(Color::GrayDark),
            text("                  v0.1.0     ") | color(Color::GrayDark),
        }));

        auto hint = [](const std::string& key, const std::string& desc) -> Element {
            return hbox(
                text(" " + key + " ") | bold | color(Color::Cyan) | size(WIDTH, EQUAL, 12),
                text(desc) | color(Color::GrayLight)
            );
        };

        Elements buf_entries;
        for (size_t i = 0; i < sm_.buffers().size(); ++i) {
            auto& buf = sm_.buffers()[i];
            std::string label = "  [" + std::to_string(i+1) + "] " + buf->name;
            if (buf->modified) label += " \u25cf";
            bool active = (i == sm_.active_buffer_idx());
            buf_entries.push_back(
                text(label) | color(active ? Color::Cyan : Color::GrayLight)
            );
        }
        if (buf_entries.empty())
            buf_entries.push_back(text("  (no buffers)") | color(Color::GrayDark));

        auto menu = vbox(make_elems({
            text(""),
            text("  ACTIONS") | bold | color(Color::White),
            separator() | color(Color::GrayDark),
            hint("<n>", "New buffer"),
            hint("<b>", "Buffer list"),
            hint("<q>", "Quit"),
            text(""),
            text("  BUFFERS") | bold | color(Color::White),
            separator() | color(Color::GrayDark),
            vbox(buf_entries),
        }));

        auto status = hbox(
            text(" slate ") | bgcolor(Color::Cyan) | color(Color::Black) | bold,
            text(" home ") | color(Color::GrayDark)
        );

        return vbox(make_elems({
            filler(),
            hbox(filler(), logo, filler()),
            hbox(filler(), menu, filler()),
            filler(),
            status,
        })) | bgcolor(Color::Black);
    }) | CatchEvent([this](Event e) -> bool {
        if (e == Event::Character('q')) { screen_.Exit(); return true; }
        if (e == Event::Character('n')) {
            auto buf = sm_.new_buffer("untitled-" + std::to_string(sm_.buffers().size()));
            sm_.push({ ScreenType::Editor, buf, "editor" });
            return true;
        }
        if (e == Event::Character('b')) {
            sm_.push({ ScreenType::BufferList, nullptr, "buffers" });
            return true;
        }
        if (e.is_character() && e.character()[0] >= '1' && e.character()[0] <= '9') {
            size_t idx = (size_t)(e.character()[0] - '1');
            if (idx < sm_.buffers().size()) {
                sm_.set_active_buffer(idx);
                sm_.push({ ScreenType::Editor, sm_.get_buffer(idx), "editor" });
                return true;
            }
        }
        return false;
    });
}

// ─── Editor Screen ────────────────────────────────────────────────────────────

Component VedApp::build_editor() {
    return Renderer([this]() -> Element {
        auto& buf = *sm_.current().buffer;

        Elements line_elems;
        for (int i = 0; i < (int)buf.lines.size(); ++i) {
            bool is_cursor_line = (i == buf.cursor_row);
            auto num = text(std::to_string(i+1) + " ") | color(Color::GrayDark) | size(WIDTH, EQUAL, 5);
            Element line_text;
            if (is_cursor_line) {
                const auto& ln = buf.lines[i];
                int col = std::min(buf.cursor_col, (int)ln.size());
                std::string before = ln.substr(0, col);
                std::string cur    = col < (int)ln.size() ? ln.substr(col, 1) : " ";
                std::string after  = col < (int)ln.size() ? ln.substr(col+1) : "";
                line_text = hbox(
                    text(before) | color(Color::White),
                    text(cur) | bgcolor(Color::Cyan) | color(Color::Black),
                    text(after) | color(Color::White)
                );
            } else {
                line_text = text(buf.lines[i]) | color(Color::GrayLight);
            }
            line_elems.push_back(hbox(num, line_text));
        }

        std::string mode_str = " NORMAL ";
        auto status = hbox(
            text(mode_str) | bgcolor(Color::Cyan) | color(Color::Black) | bold,
            text(" " + buf.name + (buf.modified ? " \u25cf" : "") + " ") | color(Color::White),
            filler(),
            text(std::to_string(buf.cursor_row+1) + ":" + std::to_string(buf.cursor_col+1) + " ") | color(Color::GrayDark)
        );

        return vbox(
            vbox(line_elems) | flex | bgcolor(Color::Black),
            status
        );
    }) | CatchEvent([this](Event e) -> bool {
        auto& buf = *sm_.current().buffer;

        if (e == Event::Escape) { sm_.pop(); return true; }
        if (e == Event::Character('h') || e == Event::ArrowLeft) {
            if (buf.cursor_col > 0) buf.cursor_col--;
            return true;
        }
        if (e == Event::Character('l') || e == Event::ArrowRight) {
            if (buf.cursor_col < (int)buf.current_line().size()) buf.cursor_col++;
            return true;
        }
        if (e == Event::Character('k') || e == Event::ArrowUp) {
            if (buf.cursor_row > 0) {
                buf.cursor_row--;
                buf.cursor_col = std::min(buf.cursor_col, (int)buf.current_line().size());
            }
            return true;
        }
        if (e == Event::Character('j') || e == Event::ArrowDown) {
            if (buf.cursor_row < (int)buf.lines.size()-1) {
                buf.cursor_row++;
                buf.cursor_col = std::min(buf.cursor_col, (int)buf.current_line().size());
            }
            return true;
        }
        return false;
    });
}

// ─── Buffer List Screen ───────────────────────────────────────────────────────

Component VedApp::build_buffer_list() {
    auto selected = std::make_shared<int>(0);
    return Renderer([this, selected]() -> Element {
        Elements entries;
        for (size_t i = 0; i < sm_.buffers().size(); ++i) {
            auto& b = sm_.buffers()[i];
            bool sel = (int)i == *selected;
            std::string label = " [" + std::to_string(i+1) + "] " + b->name + (b->modified ? " \u25cf" : "");
            entries.push_back(
                text(label) | (sel ? bgcolor(Color::Cyan) | color(Color::Black) : color(Color::GrayLight))
            );
        }

        auto status = hbox(
            text(" BUFFERS ") | bgcolor(Color::Blue) | color(Color::White) | bold,
            text(" <enter> open  <d> delete  <esc> back ") | color(Color::GrayDark)
        );

        return vbox(make_elems({
            text(" Buffer List") | bold | color(Color::White),
            separator(),
            vbox(entries) | flex,
            status,
        })) | bgcolor(Color::Black);
    }) | CatchEvent([this, selected](Event e) -> bool {
        int n = (int)sm_.buffers().size();
        if (e == Event::Escape) { sm_.pop(); return true; }
        if (e == Event::Character('k') || e == Event::ArrowUp) {
            if (*selected > 0) (*selected)--;
            return true;
        }
        if (e == Event::Character('j') || e == Event::ArrowDown) {
            if (*selected < n-1) (*selected)++;
            return true;
        }
        if (e == Event::Return) {
            size_t idx = (size_t)*selected;
            sm_.set_active_buffer(idx);
            sm_.pop();
            sm_.push({ ScreenType::Editor, sm_.get_buffer(idx), "editor" });
            return true;
        }
        return false;
    });
}

// ─── Root (screen dispatcher) ─────────────────────────────────────────────────

Component VedApp::build_root() {
    return Renderer([this]() -> Element {
        if (!sm_.has_screens()) { screen_.Exit(); return text(""); }
        auto& cur = sm_.current();
        switch (cur.type) {
            case ScreenType::Home:       return build_home()->Render();
            case ScreenType::Editor:     return build_editor()->Render();
            case ScreenType::BufferList: return build_buffer_list()->Render();
        }
        return text("");
    }) | CatchEvent([this](Event e) -> bool {
        if (!sm_.has_screens()) return false;
        auto& cur = sm_.current();
        switch (cur.type) {
            case ScreenType::Home:       return build_home()->OnEvent(e);
            case ScreenType::Editor:     return build_editor()->OnEvent(e);
            case ScreenType::BufferList: return build_buffer_list()->OnEvent(e);
        }
        return false;
    });
}

// ─── VedApp ─────────────────────────────────────────────────────────────────

VedApp::VedApp() {
    sm_.push({ ScreenType::Home, nullptr, "home" });
}

void VedApp::run() {
    auto root = build_root();
    screen_.Loop(root);
}
