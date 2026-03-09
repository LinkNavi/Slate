#include "scripting.h"
#include "app.h"
#include <wren.hpp>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

static void slate_open_file(WrenVM* vm) {
    VedApp* app = (VedApp*)wrenGetUserData(vm);
    const char* path = wrenGetSlotString(vm, 1);
    app->open_file(path);
}

static void slate_set_status(WrenVM* vm) {
    VedApp* app = (VedApp*)wrenGetUserData(vm);
    const char* msg = wrenGetSlotString(vm, 1);
    app->set_status(msg);
}

static void slate_bind_command(WrenVM* vm) {
    VedApp* app = (VedApp*)wrenGetUserData(vm);
    const char* name = wrenGetSlotString(vm, 1);
    // slot 2 is the callable wren object
    WrenHandle* receiver = wrenGetSlotHandle(vm, 2);
    WrenHandle* method   = wrenMakeCallHandle(vm, "call(_)");
    app->bind_wren_command(name, receiver, method);
}

static void slate_bind_key(WrenVM* vm) {
    VedApp* app = (VedApp*)wrenGetUserData(vm);
    const char* key    = wrenGetSlotString(vm, 1);
    WrenHandle* receiver = wrenGetSlotHandle(vm, 2);
    WrenHandle* method   = wrenMakeCallHandle(vm, "call(_,_)");
    app->bind_wren_key(key, receiver, method);
}

static void slate_close_all(WrenVM* vm) {
    VedApp* app = (VedApp*)wrenGetUserData(vm);
    app->close_all();
}

static void slate_buffer_name(WrenVM* vm) {
    VedApp* app = (VedApp*)wrenGetUserData(vm);
    wrenSetSlotString(vm, 0, app->current_buffer_name().c_str());
}

static void slate_get_line(WrenVM* vm) {
    VedApp* app = (VedApp*)wrenGetUserData(vm);
    int n = (int)wrenGetSlotDouble(vm, 1);
    wrenSetSlotString(vm, 0, app->get_line(n).c_str());
}

WrenForeignMethodFn ScriptingEngine::bind_method(WrenVM* vm, const char* module,
    const char* class_name, bool is_static, const char* signature) {
    (void)vm; (void)module; (void)is_static;
    if (std::string(class_name) == "Slate") {
        if (std::string(signature) == "openFile(_)")    return slate_open_file;
        if (std::string(signature) == "setStatus(_)")   return slate_set_status;
        if (std::string(signature) == "bindCommand(_,_)") return slate_bind_command;
        if (std::string(signature) == "bindKey(_,_)")   return slate_bind_key;
        if (std::string(signature) == "closeAll()")     return slate_close_all;
        if (std::string(signature) == "bufferName()")   return slate_buffer_name;
        if (std::string(signature) == "getLine(_)")     return slate_get_line;
    }
    return nullptr;
}

void ScriptingEngine::write(WrenVM* vm, const char* text) {
    (void)vm;
    std::cerr << text;
}

void ScriptingEngine::error_handler(WrenVM* vm, WrenErrorType type,
    const char* module, int line, const char* msg) {
    VedApp* app = (VedApp*)wrenGetUserData(vm);
    std::string err;
    if (type == WREN_ERROR_COMPILE)
        err = std::string(module) + ":" + std::to_string(line) + ": " + msg;
    else if (type == WREN_ERROR_RUNTIME)
        err = std::string("runtime: ") + msg;
    else
        err = std::string("stack: ") + msg;
    app->set_status(err);
    std::cerr << err << std::endl;
}

ScriptingEngine::ScriptingEngine(VedApp& app) : app_(app) {
    init_vm();
}

ScriptingEngine::~ScriptingEngine() {
    if (vm_) wrenFreeVM(vm_);
}

void ScriptingEngine::init_vm() {
    WrenConfiguration config;
    wrenInitConfiguration(&config);
    config.writeFn             = &ScriptingEngine::write;
    config.errorFn             = &ScriptingEngine::error_handler;
    config.bindForeignMethodFn = &ScriptingEngine::bind_method;
    config.userData            = &app_;
    vm_ = wrenNewVM(&config);

    const char* bootstrap = R"(
class Slate {
    foreign static openFile(path)
    foreign static setStatus(msg)
    foreign static bindCommand(name, fn)
    foreign static bindKey(key, fn)
    foreign static closeAll()
    foreign static bufferName()
    foreign static getLine(n)
}
)";
    wrenInterpret(vm_, "slate", bootstrap);
}

void ScriptingEngine::call(WrenCallback& cb, const std::string& arg) {
    if (!cb.receiver || !cb.method) return;
    wrenEnsureSlots(vm_, 2);
    wrenSetSlotHandle(vm_, 0, cb.receiver);
    wrenSetSlotString(vm_, 1, arg.c_str());
    wrenCall(vm_, cb.method);
}

void ScriptingEngine::load_file(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) { app_.set_status("script not found: " + path); return; }
    std::ostringstream ss;
    ss << f.rdbuf();
    auto result = wrenInterpret(vm_, path.c_str(), ss.str().c_str());
    if (result != WREN_RESULT_SUCCESS)
        app_.set_status("script error: " + path);
}

void ScriptingEngine::load_plugins_dir(const std::string& dir) {
    if (!fs::exists(dir)) return;
    for (auto& entry : fs::directory_iterator(dir)) {
        if (!entry.is_directory()) continue;
        auto main = entry.path() / "main.wren";
        if (fs::exists(main))
            load_file(main.string());
    }
}

void ScriptingEngine::error(const std::string& msg) {
    app_.set_status(msg);
}
