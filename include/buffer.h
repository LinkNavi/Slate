#pragma once
#include <string>
#include <vector>
#include <functional>

struct Buffer;
using BufferEvent = std::function<void(Buffer&)>;

struct HistoryEntry {
    std::vector<std::string> lines;
    int cursor_row, cursor_col;
};

struct Buffer {
    std::string name;
    std::string filepath;
    std::vector<std::string> lines;
    int cursor_row = 0;
    int cursor_col = 0;
    bool modified = false;

    Buffer(const std::string& n = "untitled") : name(n) {
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

    // ── Undo / Redo ─────────────────────────────────────────────────────────
    static constexpr int MAX_UNDO = 200;
    std::vector<HistoryEntry> undo_stack_;
    std::vector<HistoryEntry> redo_stack_;

    void push_undo() {
        redo_stack_.clear();
        undo_stack_.push_back({lines, cursor_row, cursor_col});
        if ((int)undo_stack_.size() > MAX_UNDO)
            undo_stack_.erase(undo_stack_.begin());
    }

    bool undo() {
        if (undo_stack_.empty()) return false;
        redo_stack_.push_back({lines, cursor_row, cursor_col});
        auto e = std::move(undo_stack_.back());
        undo_stack_.pop_back();
        lines      = std::move(e.lines);
        cursor_row = e.cursor_row;
        cursor_col = e.cursor_col;
        modified   = true;
        return true;
    }

    bool redo() {
        if (redo_stack_.empty()) return false;
        undo_stack_.push_back({lines, cursor_row, cursor_col});
        auto e = std::move(redo_stack_.back());
        redo_stack_.pop_back();
        lines      = std::move(e.lines);
        cursor_row = e.cursor_row;
        cursor_col = e.cursor_col;
        modified   = true;
        return true;
    }

    // ── Helpers ──────────────────────────────────────────────────────────────
    void clamp_cursor() {
        if (cursor_row < 0) cursor_row = 0;
        if (cursor_row >= (int)lines.size()) cursor_row = (int)lines.size() - 1;
        if (cursor_col < 0) cursor_col = 0;
        int mx = (int)current_line().size();
        if (cursor_col > mx) cursor_col = mx;
    }
};
