#include "app.h"
#include "scripting.h"
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/terminal.hpp>
using namespace ftxui;

static Elements make_elems(std::initializer_list<Element> il) {
  return Elements(il);
}

void VedApp::open_file(const std::string &path) {
  auto buf = sm_.new_buffer(path);
  buf->load(path);
  sm_.push({ScreenType::Editor, buf, "editor"});
}

Component VedApp::build_editor() {
  return Renderer([this]() -> Element {
           auto &buf = *sm_.current().buffer;
           auto [term_width, term_height] = Terminal::Size();
           int visible_lines = term_height - 1;

           if (buf.cursor_row < editor.scroll_offset)
             editor.scroll_offset = buf.cursor_row;
           if (buf.cursor_row >= editor.scroll_offset + visible_lines)
             editor.scroll_offset = buf.cursor_row - visible_lines + 1;

           int start = editor.scroll_offset;
           int end = std::min(start + visible_lines, (int)buf.lines.size());
           Elements line_elems;
           for (int i = start; i < end; ++i) {
             bool is_cursor_line = (i == buf.cursor_row);
             auto num = text(std::to_string(i + 1) + " ") |
                        color(Color::GrayDark) | size(WIDTH, EQUAL, 5);
             Element line_text;
             if (is_cursor_line) {
               const auto &ln = buf.lines[i];
               int col = std::min(buf.cursor_col, (int)ln.size());
               std::string before = ln.substr(0, col);
               std::string cur = col < (int)ln.size() ? ln.substr(col, 1) : " ";
               std::string after =
                   col < (int)ln.size() ? ln.substr(col + 1) : "";
               line_text =
                   hbox(text(before) | color(Color::White),
                        text(cur) | bgcolor(Color::Cyan) | color(Color::Black),
                        text(after) | color(Color::White));
             } else {
               line_text = text(buf.lines[i]) | color(Color::GrayLight);
             }
             line_elems.push_back(hbox(num, line_text));
           }

           std::string mode_str;
           if (editor.mode == EditorMode::EDITING)
             mode_str = " EDITING ";
           else if (editor.mode == EditorMode::VISUAL)
             mode_str = " VISUAL ";
           else
             mode_str = " NORMAL ";

           std::string status_right =
               editor.status_msg.empty()
                   ? std::to_string(buf.cursor_row + 1) + ":" +
                         std::to_string(buf.cursor_col + 1) + " "
                   : editor.status_msg + " ";

           // base status bar
           Elements status_elems;
           status_elems.push_back(text(mode_str) | bgcolor(Color::Cyan) |
                                  color(Color::Black) | bold);
           status_elems.push_back(
               text(" " + buf.name + (buf.modified ? " \u25cf" : "") + " ") |
               color(Color::White));
           status_elems.push_back(filler());
           status_elems.push_back(text(status_right) | color(Color::GrayDark));
           for (auto &hook : status_hooks_)
             status_elems.push_back(hook());
           auto status_bar = hbox(status_elems);

           // base layout
           Element base = vbox(vbox(line_elems) | flex | bgcolor(Color::Black),
                               status_bar);

           // overlays
           if (!overlay_hooks_.empty()) {
             Elements layers = {base};
             for (auto &hook : overlay_hooks_)
               layers.push_back(hook());
             base = dbox(layers);
           }

           // command bar
           if (editor.mode == EditorMode::COMMAND) {
             auto cmd_bar =
                 hbox(text(" :") | color(Color::Cyan),
                      text(editor.command_buf) | color(Color::White)) |
                 bgcolor(Color::Black);
             return vbox(vbox(line_elems) | flex | bgcolor(Color::Black),
                         cmd_bar, status_bar);
           }

           return base;
         }) |
         CatchEvent([this](Event e) -> bool {
           auto &buf = *sm_.current().buffer;

           if (e == Event::Escape) {
             if (editor.mode == EditorMode::NORMAL)
               sm_.pop();
             else
               editor.set_mode(EditorMode::NORMAL);
             return true;
           }

           if (editor.mode == EditorMode::NORMAL) {
             if (e.is_character()) {
               auto it = normal_keys_.find(e.character());
               if (it != normal_keys_.end()) {
                 it->second(buf, editor);
                 return true;
               }
             }
             if (e == Event::ArrowLeft) {
               if (buf.cursor_col > 0)
                 buf.cursor_col--;
               buf.fire_cursor_move();
               return true;
             }
             if (e == Event::ArrowRight) {
               if (buf.cursor_col < (int)buf.current_line().size())
                 buf.cursor_col++;
               buf.fire_cursor_move();
               return true;
             }
             if (e == Event::ArrowUp) {
               if (buf.cursor_row > 0) {
                 buf.cursor_row--;
                 buf.cursor_col =
                     std::min(buf.cursor_col, (int)buf.current_line().size());
               }
               buf.fire_cursor_move();
               return true;
             }
             if (e == Event::ArrowDown) {
               if (buf.cursor_row < (int)buf.lines.size() - 1) {
                 buf.cursor_row++;
                 buf.cursor_col =
                     std::min(buf.cursor_col, (int)buf.current_line().size());
               }
               buf.fire_cursor_move();
               return true;
             }
           }

           if (editor.mode == EditorMode::EDITING) {
             if (e == Event::Tab) {
               auto &ln = buf.lines[buf.cursor_row];
               ln.insert(buf.cursor_col, "    ");
               buf.cursor_col += 4;
               buf.modified = true;
               buf.fire_change();
               buf.fire_cursor_move();
               return true;
             }
             if (e.is_character()) {
               auto &ln = buf.lines[buf.cursor_row];
               ln.insert(buf.cursor_col, e.character());
               buf.cursor_col++;
               buf.modified = true;
               buf.fire_change();
               buf.fire_cursor_move();
               return true;
             }
             if (e == Event::ArrowLeft) {
               if (buf.cursor_col > 0)
                 buf.cursor_col--;
               buf.fire_cursor_move();
               return true;
             }
             if (e == Event::ArrowRight) {
               if (buf.cursor_col < (int)buf.current_line().size())
                 buf.cursor_col++;
               buf.fire_cursor_move();
               return true;
             }
             if (e == Event::ArrowUp) {
               if (buf.cursor_row > 0) {
                 buf.cursor_row--;
                 buf.cursor_col =
                     std::min(buf.cursor_col, (int)buf.current_line().size());
               }
               buf.fire_cursor_move();
               return true;
             }
             if (e == Event::ArrowDown) {
               if (buf.cursor_row < (int)buf.lines.size() - 1) {
                 buf.cursor_row++;
                 buf.cursor_col =
                     std::min(buf.cursor_col, (int)buf.current_line().size());
               }
               buf.fire_cursor_move();
               return true;
             }
             if (e == Event::Backspace) {
               auto &ln = buf.lines[buf.cursor_row];
               if (buf.cursor_col > 0) {
                 ln.erase(buf.cursor_col - 1, 1);
                 buf.cursor_col--;
                 buf.modified = true;
                 buf.fire_change();
                 buf.fire_cursor_move();
               } else if (buf.cursor_row > 0) {
                 std::string cur = ln;
                 buf.lines.erase(buf.lines.begin() + buf.cursor_row);
                 buf.cursor_row--;
                 buf.cursor_col = buf.lines[buf.cursor_row].size();
                 buf.lines[buf.cursor_row] += cur;
                 buf.modified = true;
                 buf.fire_change();
                 buf.fire_cursor_move();
               }
               return true;
             }
             if (e == Event::Return) {
               auto &ln = buf.lines[buf.cursor_row];
               std::string rest = ln.substr(buf.cursor_col);
               ln = ln.substr(0, buf.cursor_col);
               buf.cursor_row++;
               buf.lines.insert(buf.lines.begin() + buf.cursor_row, rest);
               buf.cursor_col = 0;
               buf.modified = true;
               buf.fire_change();
               buf.fire_cursor_move();
               return true;
             }
           }

           return false;
         });
}

Component VedApp::build_root() {
  return Renderer([this]() -> Element {
           if (!sm_.has_screens()) {
             screen_.Exit();
             return text("");
           }
           return build_editor()->Render();
         }) |
         CatchEvent([this](Event e) -> bool {
           if (editor.mode == EditorMode::COMMAND) {
             if (e == Event::Escape) {
               editor.set_mode(EditorMode::NORMAL);
               editor.command_buf = "";
               return true;
             }
             if (e == Event::Backspace) {
               if (!editor.command_buf.empty())
                 editor.command_buf.pop_back();
               return true;
             }
             if (e == Event::Return) {
               auto &buf = sm_.current().buffer;
               std::string cmd = editor.command_buf;
               std::string args;
               auto space = editor.command_buf.find(' ');
               if (space != std::string::npos) {
                 cmd = editor.command_buf.substr(0, space);
                 args = editor.command_buf.substr(space + 1);
               }
               auto it = commands_.find(cmd);
               if (it != commands_.end())
                 it->second(buf.get(), editor, args);
               else
                 editor.status_msg = "unknown command: " + cmd;
               editor.set_mode(EditorMode::NORMAL);
               editor.command_buf = "";
               return true;
             }
             if (e.is_character()) {
               editor.command_buf += e.character();
               return true;
             }
             return true;
           }
           if (editor.mode == EditorMode::NORMAL) {
             if (e == Event::Character(':')) {
               editor.set_mode(EditorMode::COMMAND);
               editor.command_buf = "";
               return true;
             }
           }
           if (!sm_.has_screens())
             return false;
           return build_editor()->OnEvent(e);
         });
}

VedApp::VedApp() {
  init_keybinds();
  init_commands();
  auto buf = sm_.new_buffer("untitled");
  sm_.push({ScreenType::Editor, buf, "editor"});
  
  scripting_ = std::make_unique<ScriptingEngine>(*this);
  std::string config_dir = std::string(getenv("HOME")) + "/.config/slate";
  scripting_->load_file(config_dir + "/init.wren");
  scripting_->load_plugins_dir(config_dir + "/plugins");
}

void VedApp::init_keybinds() {
  normal_keys_["h"] = [](Buffer &buf, Editor &) {
    if (buf.cursor_col > 0) {
      buf.cursor_col--;
      buf.fire_cursor_move();
    }
  };
  normal_keys_["l"] = [](Buffer &buf, Editor &) {
    if (buf.cursor_col < (int)buf.current_line().size()) {
      buf.cursor_col++;
      buf.fire_cursor_move();
    }
  };
  normal_keys_["k"] = [](Buffer &buf, Editor &) {
    if (buf.cursor_row > 0) {
      buf.cursor_row--;
      buf.cursor_col = std::min(buf.cursor_col, (int)buf.current_line().size());
      buf.fire_cursor_move();
    }
  };
  normal_keys_["j"] = [](Buffer &buf, Editor &) {
    if (buf.cursor_row < (int)buf.lines.size() - 1) {
      buf.cursor_row++;
      buf.cursor_col = std::min(buf.cursor_col, (int)buf.current_line().size());
      buf.fire_cursor_move();
    }
  };
  normal_keys_["i"] = [](Buffer &, Editor &ed) {
    ed.set_mode(EditorMode::EDITING);
  };
  normal_keys_["v"] = [](Buffer &, Editor &ed) {
    ed.set_mode(EditorMode::VISUAL);
  };
}

void VedApp::init_commands() {
  commands_["w"] = [](Buffer *buf, Editor &ed, const std::string &) {
    if (buf) {
      buf->save();
      ed.status_msg = "saved";
    } else
      ed.status_msg = "no file";
  };
  commands_["q"] = [this](Buffer *buf, Editor &ed, const std::string &) {
    if (buf && buf->modified) {
      ed.status_msg = "unsaved changes, use :q! to force";
      return;
    }
    sm_.pop();
  };
commands_["qa"] = [this](Buffer*, Editor&, const std::string&) {
    close_all();
};
  commands_["q!"] = [this](Buffer *, Editor &, const std::string &) {
    sm_.pop();
  };
  commands_["wq"] = [this](Buffer *buf, Editor &ed, const std::string &) {
    if (buf) {
      buf->save();
      ed.status_msg = "saved";
    }
    sm_.pop();
  };
  commands_["e"] = [this](Buffer *, Editor &, const std::string &args) {
    if (!args.empty())
      open_file(args);
  };
  commands_["new"] = [this](Buffer *, Editor &, const std::string &) {
    auto buf =
        sm_.new_buffer("untitled-" + std::to_string(sm_.buffers().size()));
    sm_.push({ScreenType::Editor, buf, "editor"});
  };
}
void VedApp::bind_wren_command(const std::string& name, WrenHandle* receiver, WrenHandle* method) {
    commands_[name] = [this, receiver, method](Buffer*, Editor&, const std::string& args) {
        WrenCallback cb{receiver, method};
        scripting_->call(cb, args);
    };
}

void VedApp::bind_wren_key(const std::string& key, WrenHandle* receiver, WrenHandle* method) {
    normal_keys_[key] = [this, receiver, method](Buffer& buf, Editor& ed) {
        WrenCallback cb{receiver, method};
        scripting_->call(cb, "");
    };
}

void VedApp::close_all() {
    while (sm_.has_screens()) sm_.pop();
}

std::string VedApp::current_buffer_name() {
    if (!sm_.has_screens() || !sm_.current().buffer) return "";
    return sm_.current().buffer->name;
}

std::string VedApp::get_line(int n) {
    if (!sm_.has_screens() || !sm_.current().buffer) return "";
    auto& lines = sm_.current().buffer->lines;
    if (n < 0 || n >= (int)lines.size()) return "";
    return lines[n];
}
void VedApp::bind_command(const std::string &name, CommandHandler fn) {
  commands_[name] = fn;
}

void VedApp::run() {
  auto root = build_root();
  screen_.Loop(root);
}
