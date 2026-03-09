#pragma once
#include <string>
#include <vector>
#include <functional>

struct Buffer;
using BufferEvent = std::function<void(Buffer&)>;

struct Buffer {
    std::string name;
    std::string filepath;
    std::vector<std::string> lines;
    int cursor_row = 0;
    int cursor_col = 0;
    bool modified = false;

    Buffer(const std::string& name = "untitled") : name(name) {
        lines.push_back("");
    }

    std::vector<BufferEvent> on_change;
    std::vector<BufferEvent> on_save;
    std::vector<BufferEvent> on_open;
    std::vector<BufferEvent> on_close;
    std::vector<BufferEvent> on_cursor_move;

    void fire_change()      { for (auto& f : on_change)      f(*this); }
    void fire_save()        { for (auto& f : on_save)        f(*this); }
    void fire_open()        { for (auto& f : on_open)        f(*this); }
    void fire_close()       { for (auto& f : on_close)       f(*this); }
    void fire_cursor_move() { for (auto& f : on_cursor_move) f(*this); }

    void load(const std::string& path);
    void save();
    const std::string& current_line() const { return lines[cursor_row]; }
};
