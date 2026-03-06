#pragma once
#include <string>
#include <functional>
#include <thread>

enum class AIMode { Completion, Explain, FixError, LearnMode };

struct AIClient {
    using StreamCb = std::function<void(const std::string& token)>;
    using DoneCb   = std::function<void(const std::string& full_response, bool success)>;

    void set_api_key(const std::string& key) { m_api_key = key; }
    bool has_key() const { return !m_api_key.empty(); }

    // Inline completion: sends context around cursor, streams back suggestion
    void complete(const std::string& file_content, int cursor_line, int cursor_col,
                  const std::string& lang, StreamCb stream, DoneCb done);

    // Explain selected code or error output
    void explain(const std::string& code, const std::string& context,
                 AIMode mode, StreamCb stream, DoneCb done);

    // Feed meson/GDB/compiler output, get fix suggestions
    void diagnose(const std::string& error_output, const std::string& file_content,
                  StreamCb stream, DoneCb done);

    void cancel();

private:
    void post_stream(const std::string& system_prompt, const std::string& user_msg,
                     StreamCb stream, DoneCb done);

    std::string m_api_key;
    std::thread m_worker;
    bool        m_cancel = false;

    static constexpr const char* MODEL   = "claude-sonnet-4-20250514";
    static constexpr const char* API_URL = "https://api.anthropic.com/v1/messages";
};
