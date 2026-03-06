#include "editor.h"
#include <numeric>

// ---- TextBuffer ----

TextBuffer::TextBuffer() {
    m_buf.resize(GAP_SIZE);
    m_gap_start = 0;
    m_gap_end   = GAP_SIZE;
}

TextBuffer::TextBuffer(const std::string& text) : TextBuffer() {
    insert(0, text);
}

void TextBuffer::move_gap(int pos) {
    if (pos == m_gap_start) return;
    if (pos < m_gap_start) {
        int n = m_gap_start - pos;
        std::copy_backward(m_buf.begin() + pos,
                           m_buf.begin() + m_gap_start,
                           m_buf.begin() + m_gap_end);
        m_gap_start -= n;
        m_gap_end   -= n;
    } else {
        int n = pos - m_gap_start;
        std::copy(m_buf.begin() + m_gap_end,
                  m_buf.begin() + m_gap_end + n,
                  m_buf.begin() + m_gap_start);
        m_gap_start += n;
        m_gap_end   += n;
    }
}

void TextBuffer::insert(int pos, char c) {
    if (m_gap_start == m_gap_end) {
        m_buf.insert(m_buf.begin() + m_gap_end, GAP_SIZE, '\0');
        m_gap_end += GAP_SIZE;
    }
    move_gap(pos);
    m_buf[m_gap_start++] = c;
}

void TextBuffer::insert(int pos, const std::string& text) {
    for (size_t i = 0; i < text.size(); ++i)
        insert(pos + (int)i, text[i]);
}

void TextBuffer::remove(int pos, int count) {
    move_gap(pos);
    m_gap_end = std::min(m_gap_end + count, (int)m_buf.size());
}

char TextBuffer::at(int pos) const {
    if (pos < m_gap_start) return m_buf[pos];
    return m_buf[pos + (m_gap_end - m_gap_start)];
}

int TextBuffer::size() const {
    return (int)m_buf.size() - (m_gap_end - m_gap_start);
}

std::string TextBuffer::text() const {
    std::string out;
    out.reserve(size());
    for (int i = 0; i < size(); ++i) out += at(i);
    return out;
}

int TextBuffer::line_count() const {
    int n = 1;
    for (int i = 0; i < size(); ++i)
        if (at(i) == '\n') ++n;
    return n;
}

int TextBuffer::line_start(int n) const {
    int line = 0, i = 0;
    while (i < size() && line < n) {
        if (at(i) == '\n') ++line;
        ++i;
    }
    return i;
}

std::string TextBuffer::line(int n) const {
    int start = line_start(n);
    std::string out;
    for (int i = start; i < size() && at(i) != '\n'; ++i)
        out += at(i);
    return out;
}

int TextBuffer::line_col_to_pos(int ln, int col) const {
    return line_start(ln) + col;
}

void TextBuffer::pos_to_line_col(int pos, int& ln, int& col) const {
    ln = 0; col = 0;
    for (int i = 0; i < pos && i < size(); ++i) {
        if (at(i) == '\n') { ++ln; col = 0; }
        else ++col;
    }
}

// ---- Editor ----

Editor::Editor() = default;

bool Editor::open(const std::string& path) {
    m_path = path;
    // TODO: read file into buffer
    return true;
}

bool Editor::save()                          { return false; }
bool Editor::save_as(const std::string& p)  { m_path = p; return false; }

void Editor::render(float, float, float, float) {}
void Editor::on_char(unsigned int)              {}
void Editor::on_key(int, int)                   {}

void Editor::set_ghost_text(const std::string& t) { m_ghost_text = t; }
void Editor::accept_ghost()                        { m_ghost_text.clear(); }
void Editor::clear_ghost()                         { m_ghost_text.clear(); }

const std::string& Editor::filename() const { return m_path; }