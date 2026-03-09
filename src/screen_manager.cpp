#include "screen_manager.h"

ScreenManager::ScreenManager() {


}

void ScreenManager::push(Screen s) { stack_.push_back(std::move(s)); }

void ScreenManager::pop() {
    if (!stack_.empty()) stack_.pop_back();
}

Screen& ScreenManager::current() { return stack_.back(); }

std::shared_ptr<Buffer> ScreenManager::new_buffer(const std::string& name) {
    auto buf = std::make_shared<Buffer>(name);
    buffers_.push_back(buf);
    active_buf_ = buffers_.size() - 1;
    for (auto& f : on_new_buffer) f(*buf);
    return buf;
}

std::shared_ptr<Buffer> ScreenManager::get_buffer(size_t idx) {
    if (idx < buffers_.size()) return buffers_[idx];
    return nullptr;
}

void ScreenManager::set_active_buffer(size_t idx) {
    if (idx < buffers_.size()) active_buf_ = idx;
}
