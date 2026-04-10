// app.cpp — Slate editor, all features
#include "app.h"
#include "scripting.h"
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/terminal.hpp>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <sstream>
#include <sys/select.h>
#include <unistd.h>

using namespace ftxui;

// ════════════════════════════════════════════════════════════════════════════
//  Helpers
// ════════════════════════════════════════════════════════════════════════════

static std::string mode_to_str(EditorMode m) {
    switch (m) {
    case NORMAL:  return "normal";
    case EDITING: return "insert";
    case VISUAL:  return "visual";
    case COMMAND: return "command";
    case SEARCH:  return "search";
    }
    return "normal";
}

// Return leading whitespace of a line
static std::string leading_ws(const std::string& s) {
    size_t i = 0;
    while (i < s.size() && (s[i] == ' ' || s[i] == '\t')) ++i;
    return s.substr(0, i);
}

// ════════════════════════════════════════════════════════════════════════════
//  Syntax highlighting
// ════════════════════════════════════════════════════════════════════════════

/*static*/ Color VedApp::token_color(const std::string& t) {
    if (t == "keyword")    return Color::CyanLight;
    if (t == "string")     return Color::GreenLight;
    if (t == "comment")    return Color::GrayDark;
    if (t == "number")     return Color::MagentaLight;
    if (t == "identifier") return Color::White;
    return Color::GrayLight;
}

/*static*/ std::string VedApp::file_ext(const std::string& path) {
    auto dot = path.rfind('.');
    if (dot == std::string::npos) return "";
    return path.substr(dot); // e.g. ".cpp"
}

void VedApp::add_highlight_rule(const std::string& ext,
                                const std::string& pattern,
                                const std::string& token_type) {
    try {
        highlight_rules_[ext].push_back({token_type,
            std::regex(pattern, std::regex::ECMAScript | std::regex::optimize)});
    } catch (...) {}
}

void VedApp::clear_highlight_rules(const std::string& ext) {
    highlight_rules_.erase(ext);
}

void VedApp::init_default_highlight_rules() {
    // ── C++ / C ──────────────────────────────────────────────────────────
    const std::string cpp_kw =
        R"(\b(int|void|class|struct|union|enum|if|else|for|while|do|return|)"
        R"(const|auto|bool|char|float|double|long|short|unsigned|signed|)"
        R"(static|extern|inline|virtual|override|namespace|using|template|)"
        R"(typename|new|delete|nullptr|true|false|this|public|private|)"
        R"(protected|break|continue|switch|case|default|sizeof|typedef|)"
        R"(constexpr|noexcept|explicit|friend|operator|throw|try|catch)\b)";
    for (auto& e : {std::string(".cpp"), std::string(".cc"),
                    std::string(".h"),   std::string(".hpp")}) {
        add_highlight_rule(e, R"(//.*$)",               "comment");
        add_highlight_rule(e, R"("(?:[^"\\]|\\.)*")",   "string");
        add_highlight_rule(e, R"('(?:[^'\\]|\\.)*')",   "string");
        add_highlight_rule(e, R"(#\s*\w+)",             "keyword");
        add_highlight_rule(e, cpp_kw,                   "keyword");
        add_highlight_rule(e, R"(\b0x[0-9a-fA-F]+\b)", "number");
        add_highlight_rule(e, R"(\b\d+\.?\d*[fFlL]?\b)","number");
    }

    // ── Wren ─────────────────────────────────────────────────────────────
    const std::string wren_kw =
        R"(\b(class|var|is|in|if|else|for|while|return|import|foreign|)"
        R"(static|this|super|null|true|false|new|construct|break|continue)\b)";
    add_highlight_rule(".wren", R"(//.*$)",             "comment");
    add_highlight_rule(".wren", R"("(?:[^"\\]|\\.)*")", "string");
    add_highlight_rule(".wren", wren_kw,                "keyword");
    add_highlight_rule(".wren", R"(\b\d+\.?\d*\b)",     "number");

    // .txt — no rules (plain)
}

// ── Per-line renderer ────────────────────────────────────────────────────────
Element VedApp::render_line(const Buffer& buf, int row, const std::string& ext) {
    const std::string& line = buf.lines[row];
    const bool is_cur  = (row == buf.cursor_row);
    const bool in_vis  = (editor.mode == VISUAL) &&
                         (row >= std::min(visual_anchor_row_, buf.cursor_row)) &&
                         (row <= std::max(visual_anchor_row_, buf.cursor_row));
    const int  len     = (int)line.size();
    // one extra slot for cursor-at-end-of-line
    const int  disp    = is_cur ? std::max(len, buf.cursor_col + 1) : len;

    if (disp == 0) return text("") | color(Color::GrayLight);

    struct CA { Color fg; bool cursor, visual, smatch; };
    std::vector<CA> attrs(disp, {Color::GrayLight, false, false, false});

    // ── Syntax pass ──────────────────────────────────────────────────────
    auto it_rules = highlight_rules_.find(ext);
    if (it_rules != highlight_rules_.end()) {
        for (auto& rule : it_rules->second) {
            try {
                auto beg = std::sregex_iterator(line.begin(), line.end(), rule.compiled);
                for (auto it = beg; it != std::sregex_iterator(); ++it) {
                    int s = it->position(), e = s + it->length();
                    Color col = token_color(rule.type_str);
                    for (int c = s; c < e && c < len; ++c)
                        attrs[c].fg = col;
                }
            } catch (...) {}
        }
    }

    // ── Visual selection pass ────────────────────────────────────────────
    if (in_vis) {
        for (auto& a : attrs) a.visual = true;
    }

    // ── Search match pass ────────────────────────────────────────────────
    if (!search_query_.empty()) {
        try {
            std::regex re(search_query_,
                std::regex::ECMAScript | std::regex::icase);
            auto beg = std::sregex_iterator(line.begin(), line.end(), re);
            for (auto it = beg; it != std::sregex_iterator(); ++it) {
                int s = it->position(), e = s + it->length();
                for (int c = s; c < e && c < len; ++c)
                    attrs[c].smatch = true;
            }
        } catch (...) {}
    }

    // ── Cursor pass ──────────────────────────────────────────────────────
    if (is_cur) {
        int cc = std::min(buf.cursor_col, disp - 1);
        attrs[cc].cursor = true;
    }

    // ── Build elements ───────────────────────────────────────────────────
    Elements elems;
    int i = 0;
    while (i < disp) {
        auto& a = attrs[i];
        // Find run of same attrs (cursor always alone)
        int j = a.cursor ? i + 1 : i + 1;
        while (j < disp && !attrs[j].cursor &&
               attrs[j].fg     == a.fg     &&
               attrs[j].visual == a.visual &&
               attrs[j].smatch == a.smatch) ++j;

        std::string seg = (i < len) ? line.substr(i, std::min(j, len) - i) : " ";
        if (i >= len) seg = " ";

        Element elem = text(seg);
        if (a.cursor)      elem = elem | bgcolor(Color::Cyan)   | color(Color::Black);
        else if (a.visual) elem = elem | bgcolor(Color::Blue)   | color(Color::White);
        else if (a.smatch) elem = elem | bgcolor(Color::Yellow) | color(Color::Black);
        else               elem = elem | color(a.fg);
        elems.push_back(elem);
        i = j;
    }
    if (elems.empty()) return text("") | color(Color::GrayLight);
    if (elems.size() == 1) return elems[0];
    return hbox(elems);
}

// ── Floating overlay element ─────────────────────────────────────────────────
Element VedApp::make_overlay_elem() {
    if (!overlay_active_ || overlay_text_.empty()) return text("");

    // Word-wrap at 60 chars
    constexpr int MAX_W = 60;
    Elements lines;
    std::istringstream iss(overlay_text_);
    std::string word, cur;
    auto flush = [&] {
        if (!cur.empty()) {
            lines.push_back(text(" " + cur + " ") | color(Color::White));
            cur.clear();
        }
    };
    while (iss >> word) {
        if ((int)(cur.size() + word.size() + 1) > MAX_W && !cur.empty()) flush();
        if (!cur.empty()) cur += ' ';
        cur += word;
    }
    flush();
    if (lines.empty()) lines.push_back(text(" "));

    auto box = vbox(lines) | border | bgcolor(Color::Black)
                           | size(WIDTH, LESS_THAN, MAX_W + 4);
    return vbox({filler(), hbox({filler(), box, filler()}), filler()});
}

// ════════════════════════════════════════════════════════════════════════════
//  Search
// ════════════════════════════════════════════════════════════════════════════

void VedApp::do_search(const std::string& query) {
    search_query_ = query;
    search_matches_.clear();
    search_match_idx_ = -1;
    if (query.empty() || !sm_.has_screens() || !sm_.current().buffer) return;
    auto& lines = sm_.current().buffer->lines;
    try {
        std::regex re(query, std::regex::ECMAScript | std::regex::icase);
        for (int r = 0; r < (int)lines.size(); ++r) {
            auto beg = std::sregex_iterator(lines[r].begin(), lines[r].end(), re);
            for (auto it = beg; it != std::sregex_iterator(); ++it)
                search_matches_.push_back({r, (int)it->position(), (int)it->length()});
        }
    } catch (...) {
        editor.status_msg = "invalid regex";
        return;
    }
    editor.status_msg = std::to_string(search_matches_.size()) + " match(es): " + query;
    if (!search_matches_.empty()) {
        search_match_idx_ = 0;
        jump_next_match(*sm_.current().buffer, 0);
    }
}

void VedApp::jump_next_match(Buffer& buf, int dir) {
    if (search_matches_.empty()) return;
    if (dir == 0) {
        // jump to current index
    } else {
        search_match_idx_ = (search_match_idx_ + dir +
                             (int)search_matches_.size()) %
                            (int)search_matches_.size();
    }
    auto& m = search_matches_[search_match_idx_];
    buf.cursor_row = m.row;
    buf.cursor_col = m.col;
    buf.clamp_cursor();
    buf.fire_cursor_move();
}

// ════════════════════════════════════════════════════════════════════════════
//  Pending double-key sequences (dd / yy / gg)
// ════════════════════════════════════════════════════════════════════════════

bool VedApp::handle_pending(const std::string& key, Buffer& buf, Editor& ed) {
    using clock = std::chrono::steady_clock;
    auto now = clock::now();

    if (!pending_key_.empty()) {
        bool expired = std::chrono::duration_cast<std::chrono::milliseconds>(
                           now - pending_key_time_).count() > PENDING_TIMEOUT_MS;
        if (!expired) {
            std::string combo = pending_key_ + key;
            pending_key_.clear();

            if (combo == "dd") {
                // Delete current line, yank it
                buf.push_undo();
                yank_reg_ = buf.lines[buf.cursor_row];
                buf.lines.erase(buf.lines.begin() + buf.cursor_row);
                if (buf.lines.empty()) buf.lines.push_back("");
                buf.clamp_cursor();
                buf.modified = true;
                buf.fire_change(); buf.fire_cursor_move();
                editor.status_msg = "1 line deleted";
                return true;
            }
            if (combo == "yy") {
                yank_reg_ = buf.lines[buf.cursor_row];
                editor.status_msg = "1 line yanked";
                return true;
            }
            if (combo == "gg") {
                buf.cursor_row = 0; buf.cursor_col = 0;
                buf.fire_cursor_move();
                return true;
            }
            // Unknown combo — fall through to process current key normally
        } else {
            pending_key_.clear();
        }
    }

    // Does this key start a potential combo?
    if (key == "d" || key == "y" || key == "g") {
        pending_key_     = key;
        pending_key_time_ = now;
        return true; // consumed, waiting for second key
    }
    return false;
}

// ════════════════════════════════════════════════════════════════════════════
//  build_editor
// ════════════════════════════════════════════════════════════════════════════

Component VedApp::build_editor() {
    return Renderer([this]() -> Element {
        auto& buf = *sm_.current().buffer;
        auto [term_w, term_h] = Terminal::Size();
        int visible = term_h - 2; // reserve status + optional cmd/search bar

        // Scroll to keep cursor visible
        if (buf.cursor_row < editor.scroll_offset)
            editor.scroll_offset = buf.cursor_row;
        if (buf.cursor_row >= editor.scroll_offset + visible)
            editor.scroll_offset = buf.cursor_row - visible + 1;

        int start = editor.scroll_offset;
        int end   = std::min(start + visible, (int)buf.lines.size());

        // Gutter width based on total line count (for relative line numbers)
        int total_lines = (int)buf.lines.size();
        int gutter_w = std::max(3, (int)std::to_string(total_lines).size()) + 1;

        std::string ext = file_ext(buf.filepath.empty() ? buf.name : buf.filepath);

        Elements line_elems;
        for (int i = start; i < end; ++i) {
            bool is_cur = (i == buf.cursor_row);
            // Relative line number
            std::string num_str = is_cur
                ? std::to_string(i + 1)
                : std::to_string(std::abs(i - buf.cursor_row));
            // Right-align in gutter_w - 1 cols, then space
            while ((int)num_str.size() < gutter_w - 1) num_str = " " + num_str;
            num_str += " ";

            auto num = text(num_str)
                     | color(is_cur ? Color::White : Color::GrayDark)
                     | size(WIDTH, EQUAL, gutter_w);

            line_elems.push_back(hbox(num, render_line(buf, i, ext)));
        }

        // ── Status bar ───────────────────────────────────────────────────
        std::string mode_label;
        switch (editor.mode) {
        case EDITING: mode_label = " INSERT "; break;
        case VISUAL:  mode_label = " VISUAL "; break;
        case COMMAND: mode_label = " COMMAND "; break;
        case SEARCH:  mode_label = " SEARCH "; break;
        default:      mode_label = " NORMAL "; break;
        }

        std::string status_right =
            editor.status_msg.empty()
            ? std::to_string(buf.cursor_row + 1) + ":" +
              std::to_string(buf.cursor_col + 1) + " "
            : editor.status_msg + " ";

        Elements status_elems;
        status_elems.push_back(text(mode_label) | bgcolor(Color::Cyan)
                                                 | color(Color::Black) | bold);
        status_elems.push_back(
            text(" " + buf.name + (buf.modified ? " \u25cf" : "") + " ")
            | color(Color::White));
        status_elems.push_back(filler());
        status_elems.push_back(text(status_right) | color(Color::GrayDark));
        for (auto& hook : status_hooks_)
            status_elems.push_back(hook());
        // Wren status segments
        for (auto& seg : wren_status_segs_) {
            std::string s = scripting_ ? scripting_->call_str(seg.cb) : "";
            if (!s.empty())
                status_elems.push_back(text(s) | color(Color::GrayLight));
        }
        auto status_bar = hbox(status_elems);

        // ── Build base layout ────────────────────────────────────────────
        Element editor_area = vbox(line_elems) | flex | bgcolor(Color::Black);
        Element base        = vbox(editor_area, status_bar);

        // ── Overlay ──────────────────────────────────────────────────────
        if (overlay_active_) {
            base = dbox({base, make_overlay_elem()});
        } else if (!overlay_hooks_.empty()) {
            Elements layers = {base};
            for (auto& h : overlay_hooks_) layers.push_back(h());
            base = dbox(layers);
        }

        // ── Command / Search bar ─────────────────────────────────────────
        if (editor.mode == COMMAND) {
            auto bar = hbox(text(" :") | color(Color::Cyan),
                            text(editor.command_buf) | color(Color::White))
                     | bgcolor(Color::Black);
            return vbox(editor_area, bar, status_bar);
        }
        if (editor.mode == SEARCH) {
            auto bar = hbox(text(" /") | color(Color::Yellow),
                            text(editor.search_buf) | color(Color::White))
                     | bgcolor(Color::Black);
            return vbox(editor_area, bar, status_bar);
        }

        return base;
    }) |
    CatchEvent([this](Event e) -> bool {
        auto& buf = *sm_.current().buffer;

        // Escape: dismiss overlay first, then fall through
        if (e == Event::Escape) {
            if (overlay_active_) {
                overlay_active_ = false;
                overlay_text_.clear();
                return true;
            }
            if (editor.mode == NORMAL) {
                sm_.pop();
                pending_key_.clear();
            } else {
                editor.set_mode(NORMAL);
                pending_key_.clear();
            }
            return true;
        }

        // ── NORMAL mode ──────────────────────────────────────────────────
        if (editor.mode == NORMAL) {
            // Ctrl+R = redo
            if (e.input() == "\x12") {
                if (buf.redo()) { buf.fire_change(); buf.fire_cursor_move(); }
                return true;
            }
            if (e.is_character()) {
                std::string k = e.character();

                // Double-key combos (dd, yy, gg) with pending buffer
                if (handle_pending(k, buf, editor)) return true;

                // ── u: undo ──────────────────────────────────────────────
                if (k == "u") {
                    if (buf.undo()) { buf.fire_change(); buf.fire_cursor_move(); }
                    return true;
                }
                // ── x: delete char under cursor ──────────────────────────
                if (k == "x") {
                    if (buf.cursor_col < (int)buf.current_line().size()) {
                        buf.push_undo();
                        yank_reg_ = std::string(1, buf.current_line()[buf.cursor_col]);
                        buf.lines[buf.cursor_row].erase(buf.cursor_col, 1);
                        buf.clamp_cursor();
                        buf.modified = true;
                        buf.fire_change(); buf.fire_cursor_move();
                    }
                    return true;
                }
                // ── p: paste below ───────────────────────────────────────
                if (k == "p") {
                    if (!yank_reg_.empty()) {
                        buf.push_undo();
                        buf.lines.insert(
                            buf.lines.begin() + buf.cursor_row + 1, yank_reg_);
                        buf.cursor_row++;
                        buf.cursor_col = 0;
                        buf.modified = true;
                        buf.fire_change(); buf.fire_cursor_move();
                    }
                    return true;
                }
                // ── P: paste above ───────────────────────────────────────
                if (k == "P") {
                    if (!yank_reg_.empty()) {
                        buf.push_undo();
                        buf.lines.insert(
                            buf.lines.begin() + buf.cursor_row, yank_reg_);
                        buf.cursor_col = 0;
                        buf.modified = true;
                        buf.fire_change(); buf.fire_cursor_move();
                    }
                    return true;
                }
                // ── G: file end ──────────────────────────────────────────
                if (k == "G") {
                    buf.cursor_row = (int)buf.lines.size() - 1;
                    buf.cursor_col = 0;
                    buf.fire_cursor_move();
                    return true;
                }
                // ── n: next search match ─────────────────────────────────
                if (k == "n") {
                    jump_next_match(buf, +1);
                    return true;
                }
                // ── N: prev search match ─────────────────────────────────
                if (k == "N") {
                    jump_next_match(buf, -1);
                    return true;
                }
                // ── /: enter search mode ─────────────────────────────────
                if (k == "/") {
                    editor.search_buf.clear();
                    editor.set_mode(SEARCH);
                    return true;
                }
                // ── o: new line below, enter insert ──────────────────────
                if (k == "o") {
                    buf.push_undo();
                    std::string indent = leading_ws(buf.current_line());
                    buf.lines.insert(
                        buf.lines.begin() + buf.cursor_row + 1, indent);
                    buf.cursor_row++;
                    buf.cursor_col = (int)indent.size();
                    buf.modified = true;
                    buf.fire_change(); buf.fire_cursor_move();
                    editor.set_mode(EDITING);
                    return true;
                }
                // ── O: new line above, enter insert ──────────────────────
                if (k == "O") {
                    buf.push_undo();
                    std::string indent = leading_ws(buf.current_line());
                    buf.lines.insert(
                        buf.lines.begin() + buf.cursor_row, indent);
                    buf.cursor_col = (int)indent.size();
                    buf.modified = true;
                    buf.fire_change(); buf.fire_cursor_move();
                    editor.set_mode(EDITING);
                    return true;
                }
                // ── w: next word start ───────────────────────────────────
                if (k == "w") {
                    const auto& ln = buf.current_line();
                    int col = buf.cursor_col;
                    while (col < (int)ln.size() && (std::isalnum(ln[col]) || ln[col] == '_')) col++;
                    while (col < (int)ln.size() && std::isspace(ln[col])) col++;
                    if (col >= (int)ln.size() &&
                        buf.cursor_row < (int)buf.lines.size() - 1) {
                        buf.cursor_row++; buf.cursor_col = 0;
                    } else {
                        buf.cursor_col = std::min(col, (int)ln.size());
                    }
                    buf.fire_cursor_move();
                    return true;
                }
                // ── b: prev word start ───────────────────────────────────
                if (k == "b") {
                    int col = buf.cursor_col;
                    const auto& ln = buf.current_line();
                    if (col == 0 && buf.cursor_row > 0) {
                        buf.cursor_row--;
                        buf.cursor_col = (int)buf.current_line().size();
                    } else {
                        if (col > 0) col--;
                        while (col > 0 && std::isspace(ln[col])) col--;
                        while (col > 0 && (std::isalnum(ln[col-1]) || ln[col-1] == '_')) col--;
                        buf.cursor_col = col;
                    }
                    buf.fire_cursor_move();
                    return true;
                }
                // ── e: word end ──────────────────────────────────────────
                if (k == "e") {
                    const auto& ln = buf.current_line();
                    int col = buf.cursor_col;
                    if (col < (int)ln.size()) col++;
                    while (col < (int)ln.size() && std::isspace(ln[col])) col++;
                    while (col + 1 < (int)ln.size() &&
                           (std::isalnum(ln[col+1]) || ln[col+1] == '_')) col++;
                    buf.cursor_col = std::min(col, (int)ln.size());
                    buf.fire_cursor_move();
                    return true;
                }
                // ── 0: line start ────────────────────────────────────────
                if (k == "0") {
                    buf.cursor_col = 0;
                    buf.fire_cursor_move();
                    return true;
                }
                // ── $: line end ──────────────────────────────────────────
                if (k == "$") {
                    buf.cursor_col = (int)buf.current_line().size();
                    buf.fire_cursor_move();
                    return true;
                }
                // ── i: enter insert mode (push undo snapshot) ────────────
                if (k == "i") {
                    buf.push_undo();
                    editor.set_mode(EDITING);
                    return true;
                }
                // ── v: enter visual mode ─────────────────────────────────
                if (k == "v") {
                    visual_anchor_row_ = buf.cursor_row;
                    visual_anchor_col_ = buf.cursor_col;
                    editor.set_mode(VISUAL);
                    return true;
                }

                // Lookup in user-registered normal key map
                auto it = normal_keys_.find(k);
                if (it != normal_keys_.end()) {
                    it->second(buf, editor);
                    return true;
                }
                return false;
            }
            // Arrow keys
            if (e == Event::ArrowLeft) {
                if (buf.cursor_col > 0) buf.cursor_col--;
                buf.fire_cursor_move(); return true;
            }
            if (e == Event::ArrowRight) {
                if (buf.cursor_col < (int)buf.current_line().size()) buf.cursor_col++;
                buf.fire_cursor_move(); return true;
            }
            if (e == Event::ArrowUp) {
                if (buf.cursor_row > 0) {
                    buf.cursor_row--;
                    buf.cursor_col = std::min(buf.cursor_col, (int)buf.current_line().size());
                }
                buf.fire_cursor_move(); return true;
            }
            if (e == Event::ArrowDown) {
                if (buf.cursor_row < (int)buf.lines.size() - 1) {
                    buf.cursor_row++;
                    buf.cursor_col = std::min(buf.cursor_col, (int)buf.current_line().size());
                }
                buf.fire_cursor_move(); return true;
            }
        } // end NORMAL

        // ── VISUAL mode ──────────────────────────────────────────────────────
        if (editor.mode == VISUAL) {
            if (e.is_character()) {
                std::string k = e.character();
                int lo = std::min(visual_anchor_row_, buf.cursor_row);
                int hi = std::max(visual_anchor_row_, buf.cursor_row);
                // d: delete selection
                if (k == "d") {
                    buf.push_undo();
                    // Yank selected lines
                    std::string yanked;
                    for (int r = lo; r <= hi; ++r) {
                        if (r > lo) yanked += '\n';
                        yanked += buf.lines[r];
                    }
                    yank_reg_ = yanked;
                    buf.lines.erase(buf.lines.begin() + lo,
                                    buf.lines.begin() + hi + 1);
                    if (buf.lines.empty()) buf.lines.push_back("");
                    buf.cursor_row = lo;
                    buf.clamp_cursor();
                    buf.modified = true;
                    buf.fire_change(); buf.fire_cursor_move();
                    editor.set_mode(NORMAL);
                    editor.status_msg = std::to_string(hi - lo + 1) + " lines deleted";
                    return true;
                }
                // y: yank selection
                if (k == "y") {
                    std::string yanked;
                    for (int r = lo; r <= hi; ++r) {
                        if (r > lo) yanked += '\n';
                        yanked += buf.lines[r];
                    }
                    yank_reg_ = yanked;
                    buf.cursor_row = lo;
                    buf.fire_cursor_move();
                    editor.set_mode(NORMAL);
                    editor.status_msg = std::to_string(hi - lo + 1) + " lines yanked";
                    return true;
                }
            }
            // Movement keys extend selection
            if (e == Event::ArrowUp || e == Event::Character("k")) {
                if (buf.cursor_row > 0) { buf.cursor_row--; buf.fire_cursor_move(); }
                return true;
            }
            if (e == Event::ArrowDown || e == Event::Character("j")) {
                if (buf.cursor_row < (int)buf.lines.size()-1) { buf.cursor_row++; buf.fire_cursor_move(); }
                return true;
            }
            if (e == Event::ArrowLeft || e == Event::Character("h")) {
                if (buf.cursor_col > 0) { buf.cursor_col--; buf.fire_cursor_move(); }
                return true;
            }
            if (e == Event::ArrowRight || e == Event::Character("l")) {
                if (buf.cursor_col < (int)buf.current_line().size()) { buf.cursor_col++; buf.fire_cursor_move(); }
                return true;
            }
        } // end VISUAL

        // ── EDITING mode ─────────────────────────────────────────────────────
        if (editor.mode == EDITING) {
            if (e == Event::Tab) {
                auto& ln = buf.lines[buf.cursor_row];
                ln.insert(buf.cursor_col, "    ");
                buf.cursor_col += 4;
                buf.modified = true;
                buf.fire_change(); buf.fire_cursor_move();
                return true;
            }
            if (e.is_character()) {
                std::string k = e.character();

                // Check user insert-mode bindings first
                auto it = insert_keys_.find(k);
                if (it != insert_keys_.end()) {
                    it->second(buf, editor);
                    return true;
                }

                // Auto-de-indent on '}'
                if (k == "}") {
                    auto& ln = buf.lines[buf.cursor_row];
                    bool all_ws = std::all_of(ln.begin(), ln.begin() + buf.cursor_col,
                        [](char c){ return c == ' ' || c == '\t'; });
                    if (all_ws && buf.cursor_col >= 4) {
                        ln.erase(0, 4);
                        buf.cursor_col = std::max(0, buf.cursor_col - 4);
                    }
                }

                auto& ln = buf.lines[buf.cursor_row];
                ln.insert(buf.cursor_col, k);
                buf.cursor_col++;
                buf.modified = true;
                buf.fire_change(); buf.fire_cursor_move();
                return true;
            }
            if (e == Event::ArrowLeft) {
                if (buf.cursor_col > 0) buf.cursor_col--;
                buf.fire_cursor_move(); return true;
            }
            if (e == Event::ArrowRight) {
                if (buf.cursor_col < (int)buf.current_line().size()) buf.cursor_col++;
                buf.fire_cursor_move(); return true;
            }
            if (e == Event::ArrowUp) {
                if (buf.cursor_row > 0) {
                    buf.cursor_row--;
                    buf.cursor_col = std::min(buf.cursor_col, (int)buf.current_line().size());
                }
                buf.fire_cursor_move(); return true;
            }
            if (e == Event::ArrowDown) {
                if (buf.cursor_row < (int)buf.lines.size()-1) {
                    buf.cursor_row++;
                    buf.cursor_col = std::min(buf.cursor_col, (int)buf.current_line().size());
                }
                buf.fire_cursor_move(); return true;
            }
            if (e == Event::Backspace) {
                auto& ln = buf.lines[buf.cursor_row];
                if (buf.cursor_col > 0) {
                    ln.erase(buf.cursor_col - 1, 1);
                    buf.cursor_col--;
                    buf.modified = true;
                    buf.fire_change(); buf.fire_cursor_move();
                } else if (buf.cursor_row > 0) {
                    std::string cur = ln;
                    buf.lines.erase(buf.lines.begin() + buf.cursor_row);
                    buf.cursor_row--;
                    buf.cursor_col = (int)buf.lines[buf.cursor_row].size();
                    buf.lines[buf.cursor_row] += cur;
                    buf.modified = true;
                    buf.fire_change(); buf.fire_cursor_move();
                }
                return true;
            }
            if (e == Event::Return) {
                auto& ln = buf.lines[buf.cursor_row];
                std::string rest   = ln.substr(buf.cursor_col);
                std::string before = ln.substr(0, buf.cursor_col);
                ln = before;

                // Auto-indent: carry leading whitespace
                std::string indent = leading_ws(before);
                bool extra = !before.empty() && before.back() == '{';
                if (extra) indent += "    ";

                buf.cursor_row++;
                buf.lines.insert(buf.lines.begin() + buf.cursor_row, indent + rest);
                buf.cursor_col = (int)indent.size();
                buf.modified = true;
                buf.fire_change(); buf.fire_cursor_move();
                return true;
            }
        } // end EDITING

        return false;
    });
}

// ════════════════════════════════════════════════════════════════════════════
//  build_bufferlist
// ════════════════════════════════════════════════════════════════════════════

Component VedApp::build_bufferlist() {
    return Renderer([this]() -> Element {
        auto& bufs = sm_.buffers();
        Elements entries;
        entries.push_back(text(" Open Buffers ") | bold | color(Color::Cyan));
        entries.push_back(separator());
        for (int i = 0; i < (int)bufs.size(); ++i) {
            auto& b = bufs[i];
            std::string s = " " + std::to_string(i + 1) + " " + b->name;
            if (b->modified) s += " [+]";
            if (!b->filepath.empty()) s += "  (" + b->filepath + ")";
            bool sel = (i == bufferlist_cursor_);
            auto e = text(s);
            e = sel ? e | bgcolor(Color::Blue) | color(Color::White)
                    : e | color(Color::GrayLight);
            entries.push_back(e);
        }
        auto list   = vbox(entries) | flex | bgcolor(Color::Black);
        auto status = hbox(text(" BUFFERLIST ") | bgcolor(Color::Cyan)
                                                | color(Color::Black) | bold,
                           text("  j/k: move  Enter: open  q: close")
                           | color(Color::GrayDark),
                           filler());
        return vbox(list, status);
    }) |
    CatchEvent([this](Event e) -> bool {
        if (e == Event::Character('q') || e == Event::Escape) {
            sm_.pop(); return true;
        }
        if (e == Event::Character('j') || e == Event::ArrowDown) {
            if (bufferlist_cursor_ < (int)sm_.buffers().size() - 1)
                bufferlist_cursor_++;
            return true;
        }
        if (e == Event::Character('k') || e == Event::ArrowUp) {
            if (bufferlist_cursor_ > 0) bufferlist_cursor_--;
            return true;
        }
        if (e == Event::Return) {
            auto& bufs = sm_.buffers();
            if (bufferlist_cursor_ < (int)bufs.size()) {
                auto buf = bufs[bufferlist_cursor_];
                sm_.set_active_buffer(bufferlist_cursor_);
                sm_.pop(); // close bufferlist
                sm_.push({ScreenType::Editor, buf, "editor"});
            }
            return true;
        }
        return false;
    });
}

// ════════════════════════════════════════════════════════════════════════════
//  build_root
// ════════════════════════════════════════════════════════════════════════════

Component VedApp::build_root() {
    return Renderer([this]() -> Element {
        if (!sm_.has_screens()) { screen_.Exit(); return text(""); }
        if (sm_.current().type == ScreenType::BufferList)
            return build_bufferlist()->Render();
        return build_editor()->Render();
    }) |
    CatchEvent([this](Event e) -> bool {
        if (!sm_.has_screens()) return false;

        // ── COMMAND mode (global) ────────────────────────────────────────
        if (editor.mode == COMMAND) {
            if (e == Event::Escape) {
                editor.set_mode(NORMAL);
                editor.command_buf.clear();
                return true;
            }
            if (e == Event::Backspace) {
                if (!editor.command_buf.empty()) editor.command_buf.pop_back();
                return true;
            }
            if (e == Event::Return) {
                auto& buf_ptr = sm_.current().buffer;
                std::string full = editor.command_buf;
                std::string cmd  = full, args;
                auto sp = full.find(' ');
                if (sp != std::string::npos) {
                    cmd  = full.substr(0, sp);
                    args = full.substr(sp + 1);
                }
                auto it = commands_.find(cmd);
                if (it != commands_.end())
                    it->second(buf_ptr.get(), editor, args);
                else
                    editor.status_msg = "unknown command: " + cmd;
                editor.set_mode(NORMAL);
                editor.command_buf.clear();
                return true;
            }
            if (e.is_character()) {
                editor.command_buf += e.character();
                return true;
            }
            return true;
        }

        // ── SEARCH mode (global) ─────────────────────────────────────────
        if (editor.mode == SEARCH) {
            if (e == Event::Escape) {
                editor.set_mode(NORMAL);
                editor.search_buf.clear();
                return true;
            }
            if (e == Event::Backspace) {
                if (!editor.search_buf.empty()) editor.search_buf.pop_back();
                return true;
            }
            if (e == Event::Return) {
                do_search(editor.search_buf);
                editor.set_mode(NORMAL);
                editor.search_buf.clear();
                return true;
            }
            if (e.is_character()) {
                editor.search_buf += e.character();
                return true;
            }
            return true;
        }

        // ── BufferList screen events ─────────────────────────────────────
        if (sm_.current().type == ScreenType::BufferList)
            return build_bufferlist()->OnEvent(e);

        // ── ':' enters command mode (NORMAL only) ────────────────────────
        if (editor.mode == NORMAL && e == Event::Character(':')) {
            editor.set_mode(COMMAND);
            editor.command_buf.clear();
            return true;
        }

        return build_editor()->OnEvent(e);
    });
}

// ════════════════════════════════════════════════════════════════════════════
//  Constructor
// ════════════════════════════════════════════════════════════════════════════

VedApp::VedApp() {
    // Register buffer hooks BEFORE creating any buffers
    sm_.on_new_buffer.push_back([this](Buffer& buf) {
        setup_buffer_hooks(buf);
    });

    // Mode-change → Wren callback
    editor.on_mode_change.push_back([this](EditorMode prev, EditorMode next) {
        if (scripting_ && wren_on_mode_change_.valid())
            scripting_->call2(wren_on_mode_change_,
                              mode_to_str(prev), mode_to_str(next));
    });

    init_keybinds();
    init_commands();
    init_default_highlight_rules();

    auto buf = sm_.new_buffer("untitled");
    sm_.push({ScreenType::Editor, buf, "editor"});

    scripting_ = std::make_unique<ScriptingEngine>(*this);
    std::string cfg = std::string(getenv("HOME")) + "/.config/slate";
    scripting_->load_file(cfg + "/init.wren");
    scripting_->load_plugins_dir(cfg + "/plugins");
}

void VedApp::setup_buffer_hooks(Buffer& buf) {
    buf.on_change.push_back([this](Buffer&) {
        if (scripting_ && wren_on_change_.valid())
            scripting_->call0(wren_on_change_);
    });
    buf.on_save.push_back([this](Buffer&) {
        if (scripting_ && wren_on_save_.valid())
            scripting_->call0(wren_on_save_);
    });
    buf.on_open.push_back([this](Buffer&) {
        if (scripting_ && wren_on_open_.valid())
            scripting_->call0(wren_on_open_);
    });
}

// ════════════════════════════════════════════════════════════════════════════
//  init_keybinds
// ════════════════════════════════════════════════════════════════════════════

void VedApp::init_keybinds() {
    normal_keys_["h"] = [](Buffer& b, Editor&) {
        if (b.cursor_col > 0) { b.cursor_col--; b.fire_cursor_move(); }
    };
    normal_keys_["l"] = [](Buffer& b, Editor&) {
        if (b.cursor_col < (int)b.current_line().size()) { b.cursor_col++; b.fire_cursor_move(); }
    };
    normal_keys_["k"] = [](Buffer& b, Editor&) {
        if (b.cursor_row > 0) {
            b.cursor_row--;
            b.cursor_col = std::min(b.cursor_col, (int)b.current_line().size());
            b.fire_cursor_move();
        }
    };
    normal_keys_["j"] = [](Buffer& b, Editor&) {
        if (b.cursor_row < (int)b.lines.size() - 1) {
            b.cursor_row++;
            b.cursor_col = std::min(b.cursor_col, (int)b.current_line().size());
            b.fire_cursor_move();
        }
    };
    // Note: 'i', 'v', 'o', 'O', etc. are handled directly in build_editor
    // to avoid double-handling with the pending-key logic.
}

// ════════════════════════════════════════════════════════════════════════════
//  init_commands
// ════════════════════════════════════════════════════════════════════════════

void VedApp::init_commands() {
    commands_["w"] = [](Buffer* buf, Editor& ed, const std::string&) {
        if (buf) { buf->save(); ed.status_msg = "saved"; }
        else     ed.status_msg = "no file";
    };
    commands_["q"] = [this](Buffer* buf, Editor& ed, const std::string&) {
        if (buf && buf->modified) { ed.status_msg = "unsaved changes, use :q!"; return; }
        sm_.pop();
    };
    commands_["q!"] = [this](Buffer*, Editor&, const std::string&) { sm_.pop(); };
    commands_["qa"] = [this](Buffer*, Editor&, const std::string&) { close_all(); };
    commands_["wq"] = [this](Buffer* buf, Editor& ed, const std::string&) {
        if (buf) { buf->save(); ed.status_msg = "saved"; }
        sm_.pop();
    };
    commands_["e"]  = [this](Buffer*, Editor&, const std::string& a) {
        if (!a.empty()) open_file(a);
    };
    commands_["new"] = [this](Buffer*, Editor&, const std::string&) {
        auto b = sm_.new_buffer("untitled-" + std::to_string(sm_.buffers().size()));
        sm_.push({ScreenType::Editor, b, "editor"});
    };
    // ── Buffer switching ─────────────────────────────────────────────────
    commands_["bn"] = [this](Buffer*, Editor&, const std::string&) { next_buffer(); };
    commands_["bp"] = [this](Buffer*, Editor&, const std::string&) { prev_buffer(); };
    commands_["b"]  = [this](Buffer*, Editor& ed, const std::string& a) {
        auto& bufs = sm_.buffers();
        for (size_t i = 0; i < bufs.size(); ++i) {
            if (bufs[i]->name == a || bufs[i]->filepath == a) {
                sm_.set_active_buffer(i);
                sm_.push({ScreenType::Editor, bufs[i], "editor"});
                return;
            }
        }
        ed.status_msg = "no buffer: " + a;
    };
    commands_["ls"] = [this](Buffer*, Editor&, const std::string&) {
        // Show BufferList screen
        bufferlist_cursor_ = 0;
        sm_.push({ScreenType::BufferList, nullptr, "bufferlist"});
    };
}

// ════════════════════════════════════════════════════════════════════════════
//  Buffer manipulation (Wren API surface)
// ════════════════════════════════════════════════════════════════════════════

void VedApp::open_file(const std::string& path) {
    auto buf = sm_.new_buffer(path);
    buf->load(path);
    sm_.push({ScreenType::Editor, buf, "editor"});
}

int VedApp::line_count() {
    if (!sm_.has_screens() || !sm_.current().buffer) return 0;
    return (int)sm_.current().buffer->lines.size();
}

std::string VedApp::get_line(int n) {
    if (!sm_.has_screens() || !sm_.current().buffer) return "";
    auto& lines = sm_.current().buffer->lines;
    if (n < 0 || n >= (int)lines.size()) return "";
    return lines[n];
}

void VedApp::set_line(int n, const std::string& s) {
    if (!sm_.has_screens() || !sm_.current().buffer) return;
    auto& buf = *sm_.current().buffer;
    n = std::clamp(n, 0, (int)buf.lines.size() - 1);
    buf.lines[n] = s;
    buf.modified = true;
    buf.fire_change();
}

void VedApp::insert_line(int n, const std::string& s) {
    if (!sm_.has_screens() || !sm_.current().buffer) return;
    auto& buf = *sm_.current().buffer;
    n = std::clamp(n, 0, (int)buf.lines.size());
    buf.lines.insert(buf.lines.begin() + n, s);
    buf.modified = true;
    buf.fire_change();
}

void VedApp::delete_line(int n) {
    if (!sm_.has_screens() || !sm_.current().buffer) return;
    auto& buf = *sm_.current().buffer;
    if (n < 0 || n >= (int)buf.lines.size()) return;
    buf.lines.erase(buf.lines.begin() + n);
    if (buf.lines.empty()) buf.lines.push_back("");
    buf.clamp_cursor();
    buf.modified = true;
    buf.fire_change(); buf.fire_cursor_move();
}

std::string VedApp::get_cursor() {
    if (!sm_.has_screens() || !sm_.current().buffer) return "0:0";
    auto& buf = *sm_.current().buffer;
    return std::to_string(buf.cursor_row) + ":" + std::to_string(buf.cursor_col);
}

void VedApp::set_cursor(int row, int col) {
    if (!sm_.has_screens() || !sm_.current().buffer) return;
    auto& buf = *sm_.current().buffer;
    buf.cursor_row = row;
    buf.cursor_col = col;
    buf.clamp_cursor();
    buf.fire_cursor_move();
}

std::string VedApp::get_mode_str() { return mode_to_str(editor.mode); }

void VedApp::set_mode_str(const std::string& s) {
    if      (s == "normal")  editor.set_mode(NORMAL);
    else if (s == "insert")  editor.set_mode(EDITING);
    else if (s == "visual")  editor.set_mode(VISUAL);
    else if (s == "command") editor.set_mode(COMMAND);
    else if (s == "search")  editor.set_mode(SEARCH);
}

void VedApp::set_search_query(const std::string& s) { do_search(s); }

bool VedApp::is_modified() {
    if (!sm_.has_screens() || !sm_.current().buffer) return false;
    return sm_.current().buffer->modified;
}

std::string VedApp::file_path() {
    if (!sm_.has_screens() || !sm_.current().buffer) return "";
    return sm_.current().buffer->filepath;
}

void VedApp::do_undo() {
    if (!sm_.has_screens() || !sm_.current().buffer) return;
    auto& buf = *sm_.current().buffer;
    if (buf.undo()) { buf.fire_change(); buf.fire_cursor_move(); }
}

void VedApp::do_redo() {
    if (!sm_.has_screens() || !sm_.current().buffer) return;
    auto& buf = *sm_.current().buffer;
    if (buf.redo()) { buf.fire_change(); buf.fire_cursor_move(); }
}

void VedApp::do_push_undo() {
    if (!sm_.has_screens() || !sm_.current().buffer) return;
    sm_.current().buffer->push_undo();
}

std::string VedApp::exec_cmd(const std::string& cmd) {
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "";
    int fd = fileno(pipe);
    std::string result;
    char rbuf[256];
    struct timeval tv{2, 0};
    while (true) {
        fd_set fds; FD_ZERO(&fds); FD_SET(fd, &fds);
        int ret = select(fd + 1, &fds, nullptr, nullptr, &tv);
        if (ret <= 0) break;
        if (!fgets(rbuf, sizeof(rbuf), pipe)) break;
        result += rbuf;
    }
    pclose(pipe);
    return result;
}

void VedApp::save_file() {
    if (!sm_.has_screens() || !sm_.current().buffer) return;
    sm_.current().buffer->save();
    editor.status_msg = "saved";
}

void VedApp::close_buffer() { if (sm_.has_screens()) sm_.pop(); }

void VedApp::next_buffer() {
    auto& bufs = sm_.buffers();
    if (bufs.empty()) return;
    size_t idx = (sm_.active_buffer_idx() + 1) % bufs.size();
    sm_.set_active_buffer(idx);
    sm_.push({ScreenType::Editor, bufs[idx], "editor"});
}

void VedApp::prev_buffer() {
    auto& bufs = sm_.buffers();
    if (bufs.empty()) return;
    size_t idx = sm_.active_buffer_idx();
    idx = (idx == 0) ? bufs.size() - 1 : idx - 1;
    sm_.set_active_buffer(idx);
    sm_.push({ScreenType::Editor, bufs[idx], "editor"});
}

void VedApp::new_buffer_named(const std::string& name) {
    auto b = sm_.new_buffer(name);
    sm_.push({ScreenType::Editor, b, "editor"});
}

std::string VedApp::current_buffer_name() {
    if (!sm_.has_screens() || !sm_.current().buffer) return "";
    return sm_.current().buffer->name;
}

void VedApp::close_all() { while (sm_.has_screens()) sm_.pop(); }

// ════════════════════════════════════════════════════════════════════════════
//  Overlay
// ════════════════════════════════════════════════════════════════════════════

void VedApp::set_overlay(const std::string& text) {
    overlay_text_   = text;
    overlay_active_ = !text.empty();
}

// ════════════════════════════════════════════════════════════════════════════
//  Wren bind helpers
// ════════════════════════════════════════════════════════════════════════════

void VedApp::bind_command(const std::string& name, CommandHandler fn) {
    commands_[name] = fn;
}

void VedApp::bind_wren_command(const std::string& name, WrenHandle* r, WrenHandle* m) {
    commands_[name] = [this, r, m](Buffer*, Editor&, const std::string& args) {
        WrenCallback cb{r, m};
        scripting_->call(cb, args);
    };
}

void VedApp::bind_wren_key(const std::string& key, WrenHandle* r, WrenHandle* m) {
    normal_keys_[key] = [this, r, m](Buffer&, Editor&) {
        WrenCallback cb{r, m};
        scripting_->call(cb, "");
    };
}

void VedApp::bind_wren_insert_key(const std::string& key, WrenHandle* r, WrenHandle* m) {
    insert_keys_[key] = [this, r, m](Buffer&, Editor&) {
        WrenCallback cb{r, m};
        scripting_->call(cb, "");
    };
}

void VedApp::bind_wren_on_change(WrenHandle* r, WrenHandle* m) {
    wren_on_change_ = {r, m};
}
void VedApp::bind_wren_on_save(WrenHandle* r, WrenHandle* m) {
    wren_on_save_ = {r, m};
}
void VedApp::bind_wren_on_open(WrenHandle* r, WrenHandle* m) {
    wren_on_open_ = {r, m};
}
void VedApp::bind_wren_on_mode_change(WrenHandle* r, WrenHandle* m) {
    wren_on_mode_change_ = {r, m};
}
void VedApp::add_wren_status_seg(WrenHandle* r, WrenHandle* m) {
    wren_status_segs_.push_back({{r, m}});
}

// ════════════════════════════════════════════════════════════════════════════
//  run
// ════════════════════════════════════════════════════════════════════════════

void VedApp::run() {
    auto root = build_root();
    screen_.Loop(root);
}
