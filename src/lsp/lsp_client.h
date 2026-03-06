#pragma once
#include <string>
#include <vector>
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>

struct Diagnostic {
    int line, col, end_col;
    std::string message;
    int severity; // 1=error 2=warn 3=info
};

struct CompletionItem {
    std::string label;
    std::string detail;
    std::string insert_text;
    int kind; // LSP CompletionItemKind
};

struct LSPClient {
    using DiagCb       = std::function<void(const std::string& uri, std::vector<Diagnostic>)>;
    using CompletionCb = std::function<void(std::vector<CompletionItem>)>;

    LSPClient();
    ~LSPClient();

    bool start(const std::string& server_path = "clangd"); // e.g. "clangd"
    void stop();
    bool is_running() const { return m_running; }

    void open_file(const std::string& uri, const std::string& text, const std::string& lang = "cpp");
    void change_file(const std::string& uri, const std::string& text, int version);
    void close_file(const std::string& uri);

    void request_completion(const std::string& uri, int line, int col, CompletionCb cb);
    void request_hover(const std::string& uri, int line, int col, std::function<void(std::string)> cb);

    void on_diagnostics(DiagCb cb) { m_diag_cb = cb; }

private:
    void read_loop();
    void dispatch(const std::string& json);
    void send(const std::string& json);
    std::string make_header(const std::string& body);

    int          m_req_id = 1;
    std::atomic<bool> m_running{false};

#if defined(_WIN32)
    void* m_stdin_write  = nullptr;
    void* m_stdout_read  = nullptr;
    void* m_process      = nullptr;
#else
    int   m_stdin_fd  = -1;
    int   m_stdout_fd = -1;
    pid_t m_pid       = -1;
#endif

    std::thread m_reader;
    std::mutex  m_send_mtx;

    DiagCb       m_diag_cb;
    std::mutex   m_cb_mtx;

    // Pending request callbacks keyed by id
    std::unordered_map<int, std::function<void(const std::string&)>> m_pending;
};
