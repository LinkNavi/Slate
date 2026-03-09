#pragma once
#include <string>
#include <vector>

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

    void load(const std::string& path);
    void save();
    const std::string& current_line() const { return lines[cursor_row]; }
};
