#pragma once
#include <string>
#include <vector>
#include "../theme/theme.h"
#include "../lsp/lsp_client.h"

// Simple gap-buffer based text buffer
struct TextBuffer {
    TextBuffer();
    explicit TextBuffer(const std::string& text);

    void insert(int pos, char c);
    void insert(int pos, const std::string& text);
    void remove(int pos, int count = 1);

    char        at(int pos) const;
    std::string text() const;
    std::string line(int n) const;
    int         line_count() const;
    int         line_start(int n) const;
    int         line_col_to_pos(int line, int col) const;
    void        pos_to_line_col(int pos, int& line, int& col) const;
    int         size() const;

private:
    void move_gap(int pos);
    std::vector<char> m_buf;
    int m_gap_start = 0;
    int m_gap_end   = 64;
    static constexpr int GAP_SIZE = 64;
};

enum class TokenType {
    Normal, Keyword, Type, String, Comment,
    Number, Function, Preprocessor, Operator
};

struct Token {
    int       start, end;
    TokenType type;
};

struct SyntaxHighlighter {
    std::vector<Token> tokenize(const std::string& text, const std::string& lang);
};

struct Editor {
    Editor();

    bool open(const std::string& path);
    bool save();
    bool save_as(const std::string& path);
    bool is_modified() const { return m_modified; }

    void set_lsp(LSPClient* lsp) { m_lsp = lsp; }
    void set_theme(const Theme* theme) { m_theme = theme; }

    // Called each frame to render the editor into a region
    void render(float x, float y, float w, float h);

    // Input handling
    void on_char(unsigned int codepoint);
    void on_key(int key, int mods);  // sokol key codes

    const std::string& path()     const { return m_path; }
    const std::string& filename() const;

    // Ghost text from AI completion
    void set_ghost_text(const std::string& text);
    void accept_ghost();
    void clear_ghost();

private:
    void ensure_cursor_visible();
    void rerender_tokens();
    Color token_color(TokenType t) const;

    TextBuffer        m_buf;
    std::string       m_path;
    std::string       m_lang = "cpp";
    int               m_cursor  = 0;
    int               m_sel_start = -1;
    int               m_scroll_line = 0;
    float             m_scroll_px   = 0;
    bool              m_modified    = false;

    std::vector<Token>    m_tokens;
    SyntaxHighlighter     m_highlighter;

    std::string       m_ghost_text;
    int               m_ghost_pos = -1;

    std::vector<Diagnostic> m_diagnostics;

    LSPClient*        m_lsp   = nullptr;
    const Theme*      m_theme = nullptr;
    int               m_lsp_version = 0;

    // Idle timer for LSP + AI triggers
    uint64_t          m_last_edit_tick = 0;
};
