#pragma once
#include "buffer.h"
#include <vector>
#include <memory>
#include <functional>

enum class ScreenType {
    Home,
    Editor,
    BufferList,
};

struct Screen {
    ScreenType type;
    std::shared_ptr<Buffer> buffer; // null for non-editor screens
    std::string title;
};

class ScreenManager {
public:
    ScreenManager();

    void push(Screen s);
    void pop();
    Screen& current();
    bool has_screens() const { return !stack_.empty(); }

    // Buffer pool shared across all editor screens
    std::vector<std::shared_ptr<Buffer>>& buffers() { return buffers_; }
    std::shared_ptr<Buffer> new_buffer(const std::string& name = "untitled");
    std::shared_ptr<Buffer> get_buffer(size_t idx);
    size_t active_buffer_idx() const { return active_buf_; }
    void set_active_buffer(size_t idx);

private:
    std::vector<Screen> stack_;
    std::vector<std::shared_ptr<Buffer>> buffers_;
    size_t active_buf_ = 0;
};
