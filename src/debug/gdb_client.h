#pragma once
#include <string>
#include <vector>
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>
#include <unordered_map>

struct StackFrame {
    int         level;
    std::string func;
    std::string file;
    int         line;
    std::string addr;
};

struct Variable {
    std::string name;
    std::string value;
    std::string type;
};

struct Breakpoint {
    int         id;
    std::string file;
    int         line;
    bool        enabled;
};

struct GDBClient {
    using StopCb   = std::function<void(const std::string& reason, const std::string& file, int line)>;
    using OutputCb = std::function<void(const std::string& text)>;

    GDBClient();
    ~GDBClient();

    bool start(const std::string& executable);
    void stop();
    bool is_running() const { return m_running; }

    // Execution control
    void run();
    void cont();
    void step_over();   // next
    void step_into();   // step
    void step_out();    // finish
    void interrupt();   // SIGINT

    // Breakpoints
    int  set_breakpoint(const std::string& file, int line);
    void remove_breakpoint(int id);
    void toggle_breakpoint(int id, bool enabled);
    const std::vector<Breakpoint>& breakpoints() const { return m_breakpoints; }

    // Inspection
    void get_stack(std::function<void(std::vector<StackFrame>)> cb);
    void get_locals(std::function<void(std::vector<Variable>)> cb);
    void evaluate(const std::string& expr, std::function<void(std::string)> cb);

    void on_stop(StopCb cb)     { m_stop_cb = cb; }
    void on_output(OutputCb cb) { m_out_cb  = cb; }

private:
    void read_loop();
    void dispatch(const std::string& line);
    void send(const std::string& cmd);
    std::string next_token();

    std::atomic<bool> m_running{false};
    int  m_token = 1;

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

    StopCb   m_stop_cb;
    OutputCb m_out_cb;

    std::vector<Breakpoint> m_breakpoints;
    std::unordered_map<int, std::function<void(const std::string&)>> m_pending;
};
