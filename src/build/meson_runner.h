#pragma once
#include <string>
#include <vector>
#include <functional>
#include <thread>
#include <atomic>

enum class BuildStatus { Idle, Building, Success, Failed };
enum class TestStatus  { Idle, Running, Passed, Failed };

struct TestResult {
    std::string name;
    bool        passed;
    std::string output;
    double      duration_s;
};

struct WrapEntry {
    std::string name;
    std::string version;
    std::string url;
};

struct MesonRunner {
    using OutputCb = std::function<void(const std::string& line)>;
    using DoneCb   = std::function<void(bool success)>;

    // Build
    void configure(const std::string& src_dir, const std::string& build_dir,
                   const std::vector<std::string>& opts, DoneCb done);
    void build(const std::string& build_dir, OutputCb out, DoneCb done);
    void clean(const std::string& build_dir, DoneCb done);

    // Tests
    void run_tests(const std::string& build_dir, const std::string& filter,
                   OutputCb out, std::function<void(std::vector<TestResult>)> done);

    // Wrap DB
    void wrap_list(std::function<void(std::vector<WrapEntry>)> cb);
    void wrap_install(const std::string& src_dir, const std::string& name, DoneCb done);
    void wrap_update(const std::string& src_dir, DoneCb done);

    // Project scaffolding
    static bool create_project(const std::string& dir, const std::string& name,
                                const std::string& lang, const std::string& type);

    BuildStatus build_status() const { return m_build_status; }
    TestStatus  test_status()  const { return m_test_status; }

    void cancel();

private:
    void run_process(const std::vector<std::string>& args, const std::string& cwd,
                     OutputCb out, DoneCb done);

    std::atomic<BuildStatus> m_build_status{BuildStatus::Idle};
    std::atomic<TestStatus>  m_test_status{TestStatus::Idle};
    std::thread m_worker;
    std::atomic<bool> m_cancel{false};
};
