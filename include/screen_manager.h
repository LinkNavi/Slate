#pragma once
#include "buffer.h"
#include <vector>
#include <memory>
#include <functional>

enum class ScreenType { Editor, BufferList };

// ── Split tree ────────────────────────────────────────────────────────────────
enum class SplitDir { None, Vertical, Horizontal };

struct SplitNode {
    SplitDir dir = SplitDir::None; // None = leaf

    // Leaf data
    std::shared_ptr<Buffer> buffer;
    int scroll_offset = 0;

    // Internal node data
    std::shared_ptr<SplitNode> a, b; // a=left/top, b=right/bottom
    float ratio = 0.5f;              // fraction given to 'a'

    bool is_leaf() const { return dir == SplitDir::None; }
};

struct Screen {
    ScreenType type;
    std::shared_ptr<SplitNode> split_root;
    std::shared_ptr<Buffer>    buffer;     // for BufferList
    std::string title;
};

using NewBufferEvent = std::function<void(Buffer&)>;

class ScreenManager {
public:
    ScreenManager();

    void push(Screen s);
    void pop();
    Screen& current();
    bool has_screens() const { return !stack_.empty(); }

    std::vector<std::shared_ptr<Buffer>>& buffers() { return buffers_; }
    std::shared_ptr<Buffer> new_buffer(const std::string& name = "untitled");
    std::shared_ptr<Buffer> get_buffer(size_t idx);
    size_t active_buffer_idx() const { return active_buf_; }
    void set_active_buffer(size_t idx);

    // Split API
    SplitNode* focused_leaf() const { return focused_; }
    void set_focused(SplitNode* n) { focused_ = n; }

    // Split the focused leaf. Returns the new leaf.
    SplitNode* split(SplitDir dir, std::shared_ptr<Buffer> buf);
    // Close the focused leaf. Returns false if it was the last pane.
    bool close_focused();
    // Cycle focus to next/prev leaf (dir: +1 or -1)
    void cycle_focus(int dir);

    // Collect all leaves in order
    std::vector<SplitNode*> all_leaves();

    std::vector<NewBufferEvent> on_new_buffer;

private:
    std::vector<Screen> stack_;
    std::vector<std::shared_ptr<Buffer>> buffers_;
    size_t active_buf_ = 0;
    SplitNode* focused_ = nullptr;

    void collect_leaves(SplitNode* n, std::vector<SplitNode*>& out);
    // Remove node containing target from tree rooted at root, replacing
    // the parent with the sibling. Returns new root.
    std::shared_ptr<SplitNode> remove_leaf(
        std::shared_ptr<SplitNode> root, SplitNode* target,
        SplitNode*& new_focus);
};
