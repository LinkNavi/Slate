#include "screen_manager.h"

ScreenManager::ScreenManager() {}

void ScreenManager::push(Screen s) { stack_.push_back(std::move(s)); }
void ScreenManager::pop()          { if (!stack_.empty()) stack_.pop_back(); }
Screen& ScreenManager::current()   { return stack_.back(); }

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

// ── Leaf collection ───────────────────────────────────────────────────────────

void ScreenManager::collect_leaves(SplitNode* n, std::vector<SplitNode*>& out) {
    if (!n) return;
    if (n->is_leaf()) { out.push_back(n); return; }
    collect_leaves(n->a.get(), out);
    collect_leaves(n->b.get(), out);
}

std::vector<SplitNode*> ScreenManager::all_leaves() {
    std::vector<SplitNode*> out;
    if (has_screens() && current().split_root)
        collect_leaves(current().split_root.get(), out);
    return out;
}

// ── Split ─────────────────────────────────────────────────────────────────────

SplitNode* ScreenManager::split(SplitDir dir, std::shared_ptr<Buffer> buf) {
    if (!has_screens()) return nullptr;
    auto& root = current().split_root;
    if (!root || !focused_) return nullptr;

    // Build new leaf
    auto new_leaf = std::make_shared<SplitNode>();
    new_leaf->buffer = buf;

    // Replace focused_ leaf with an internal node containing focused_ and new_leaf
    // We need to find focused_ in the tree and wrap it.
    std::function<bool(std::shared_ptr<SplitNode>&)> wrap =
        [&](std::shared_ptr<SplitNode>& node) -> bool {
        if (!node) return false;
        if (node.get() == focused_) {
            auto old = node; // current leaf becomes 'a'
            node = std::make_shared<SplitNode>();
            node->dir   = dir;
            node->ratio = 0.5f;
            node->a     = old;
            node->b     = new_leaf;
            return true;
        }
        if (!node->is_leaf())
            return wrap(node->a) || wrap(node->b);
        return false;
    };

    wrap(root);
    focused_ = new_leaf.get();
    return focused_;
}

// ── Close focused ─────────────────────────────────────────────────────────────

std::shared_ptr<SplitNode> ScreenManager::remove_leaf(
    std::shared_ptr<SplitNode> root, SplitNode* target, SplitNode*& new_focus)
{
    if (!root) return nullptr;
    if (root->is_leaf()) {
        // Can't remove the only leaf
        return root;
    }
    // Check if a child is the target leaf
    if (root->a && root->a->is_leaf() && root->a.get() == target) {
        new_focus = root->b->is_leaf()
                  ? root->b.get()
                  : nullptr; // will fix up after
        return root->b;
    }
    if (root->b && root->b->is_leaf() && root->b.get() == target) {
        new_focus = root->a->is_leaf()
                  ? root->a.get()
                  : nullptr;
        return root->a;
    }
    root->a = remove_leaf(root->a, target, new_focus);
    root->b = remove_leaf(root->b, target, new_focus);
    return root;
}

bool ScreenManager::close_focused() {
    if (!has_screens() || !focused_) return false;
    auto& root = current().split_root;
    if (!root) return false;
    // Only one leaf — can't close
    auto leaves = all_leaves();
    if (leaves.size() <= 1) return false;

    SplitNode* new_focus = nullptr;
    root = remove_leaf(root, focused_, new_focus);

    // If new_focus wasn't set (sibling was internal), pick first leaf
    if (!new_focus) {
        auto lv = all_leaves();
        new_focus = lv.empty() ? nullptr : lv[0];
    }
    focused_ = new_focus;
    return true;
}

// ── Cycle focus ───────────────────────────────────────────────────────────────

void ScreenManager::cycle_focus(int dir) {
    auto leaves = all_leaves();
    if (leaves.size() < 2) return;
    for (int i = 0; i < (int)leaves.size(); ++i) {
        if (leaves[i] == focused_) {
            int next = (i + dir + (int)leaves.size()) % (int)leaves.size();
            focused_ = leaves[next];
            return;
        }
    }
    focused_ = leaves[0];
}
