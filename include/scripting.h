#pragma once
#include <string>
#include <vector>
#include <wren.hpp>

class VedApp;

struct WrenCallback {
    WrenHandle* receiver = nullptr;
    WrenHandle* method   = nullptr;
    bool valid() const { return receiver && method; }
};

class ScriptingEngine {
public:
    ScriptingEngine(VedApp& app);
    ~ScriptingEngine();

    void load_file(const std::string& path);
    void load_plugins_dir(const std::string& dir);
    void error(const std::string& msg);

    // call fn(arg) — one string arg, no return value used
    void call(WrenCallback& cb, const std::string& arg);
    // call fn() — zero args, no return value used
    void call0(WrenCallback& cb);
    // call fn(a, b) — two string args, no return value used
    void call2(WrenCallback& cb, const std::string& a, const std::string& b);
    // call fn() — zero args, return string from slot 0
    std::string call_str(WrenCallback& cb);

    WrenVM* vm() { return vm_; }

private:
    VedApp& app_;
    WrenVM* vm_ = nullptr;

    void init_vm();
    static WrenForeignMethodFn bind_method(WrenVM* vm, const char* module,
        const char* class_name, bool is_static, const char* signature);
    static void write(WrenVM* vm, const char* text);
    static void error_handler(WrenVM* vm, WrenErrorType type,
        const char* module, int line, const char* msg);
};
