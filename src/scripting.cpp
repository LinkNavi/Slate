// scripting.cpp — Wren scripting engine + all foreign method bindings
#include "scripting.h"
#include "app.h"
#include <wren.hpp>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

// ════════════════════════════════════════════════════════════════════════════
//  Foreign method implementations
// ════════════════════════════════════════════════════════════════════════════

// ── Existing ─────────────────────────────────────────────────────────────────

static void slate_api_version(WrenVM* vm) {
    wrenSetSlotString(vm, 0, "1.0");
}

static void slate_open_file(WrenVM* vm) {
    VedApp* app = (VedApp*)wrenGetUserData(vm);
    app->open_file(wrenGetSlotString(vm, 1));
}

static void slate_set_status(WrenVM* vm) {
    VedApp* app = (VedApp*)wrenGetUserData(vm);
    app->set_status(wrenGetSlotString(vm, 1));
}

static void slate_close_all(WrenVM* vm) {
    ((VedApp*)wrenGetUserData(vm))->close_all();
}

static void slate_buffer_name(WrenVM* vm) {
    wrenSetSlotString(vm, 0,
        ((VedApp*)wrenGetUserData(vm))->current_buffer_name().c_str());
}

static void slate_get_line(WrenVM* vm) {
    VedApp* app = (VedApp*)wrenGetUserData(vm);
    int n = (int)wrenGetSlotDouble(vm, 1);
    wrenSetSlotString(vm, 0, app->get_line(n).c_str());
}

// ── Buffer access ─────────────────────────────────────────────────────────────

static void slate_line_count(WrenVM* vm) {
    wrenSetSlotDouble(vm, 0, (double)((VedApp*)wrenGetUserData(vm))->line_count());
}

static void slate_set_line(WrenVM* vm) {
    VedApp* app = (VedApp*)wrenGetUserData(vm);
    int n = (int)wrenGetSlotDouble(vm, 1);
    const char* s = wrenGetSlotString(vm, 2);
    app->set_line(n, s);
}

static void slate_insert_line(WrenVM* vm) {
    VedApp* app = (VedApp*)wrenGetUserData(vm);
    int n = (int)wrenGetSlotDouble(vm, 1);
    const char* s = wrenGetSlotString(vm, 2);
    app->insert_line(n, s);
}

static void slate_delete_line(WrenVM* vm) {
    VedApp* app = (VedApp*)wrenGetUserData(vm);
    int n = (int)wrenGetSlotDouble(vm, 1);
    app->delete_line(n);
}

static void slate_get_cursor(WrenVM* vm) {
    wrenSetSlotString(vm, 0, ((VedApp*)wrenGetUserData(vm))->get_cursor().c_str());
}

static void slate_set_cursor(WrenVM* vm) {
    VedApp* app = (VedApp*)wrenGetUserData(vm);
    int row = (int)wrenGetSlotDouble(vm, 1);
    int col = (int)wrenGetSlotDouble(vm, 2);
    app->set_cursor(row, col);
}

static void slate_get_yank(WrenVM* vm) {
    wrenSetSlotString(vm, 0, ((VedApp*)wrenGetUserData(vm))->get_yank_reg().c_str());
}

static void slate_set_yank(WrenVM* vm) {
    ((VedApp*)wrenGetUserData(vm))->set_yank_reg(wrenGetSlotString(vm, 1));
}

// ── Editor state ──────────────────────────────────────────────────────────────

static void slate_get_mode(WrenVM* vm) {
    wrenSetSlotString(vm, 0, ((VedApp*)wrenGetUserData(vm))->get_mode_str().c_str());
}

static void slate_set_mode(WrenVM* vm) {
    ((VedApp*)wrenGetUserData(vm))->set_mode_str(wrenGetSlotString(vm, 1));
}

static void slate_get_search(WrenVM* vm) {
    wrenSetSlotString(vm, 0, ((VedApp*)wrenGetUserData(vm))->get_search_query().c_str());
}

static void slate_set_search(WrenVM* vm) {
    ((VedApp*)wrenGetUserData(vm))->set_search_query(wrenGetSlotString(vm, 1));
}

static void slate_is_modified(WrenVM* vm) {
    wrenSetSlotDouble(vm, 0, ((VedApp*)wrenGetUserData(vm))->is_modified() ? 1.0 : 0.0);
}

static void slate_file_path(WrenVM* vm) {
    wrenSetSlotString(vm, 0, ((VedApp*)wrenGetUserData(vm))->file_path().c_str());
}

// ── Undo / Redo ───────────────────────────────────────────────────────────────

static void slate_undo(WrenVM* vm) {
    ((VedApp*)wrenGetUserData(vm))->do_undo();
}

static void slate_redo(WrenVM* vm) {
    ((VedApp*)wrenGetUserData(vm))->do_redo();
}

static void slate_push_undo(WrenVM* vm) {
    ((VedApp*)wrenGetUserData(vm))->do_push_undo();
}

// ── Actions ───────────────────────────────────────────────────────────────────

static void slate_exec(WrenVM* vm) {
    VedApp* app = (VedApp*)wrenGetUserData(vm);
    std::string r = app->exec_cmd(wrenGetSlotString(vm, 1));
    wrenSetSlotString(vm, 0, r.c_str());
}

static void slate_save_file(WrenVM* vm) {
    ((VedApp*)wrenGetUserData(vm))->save_file();
}

static void slate_close_buffer(WrenVM* vm) {
    ((VedApp*)wrenGetUserData(vm))->close_buffer();
}

static void slate_next_buffer(WrenVM* vm) {
    ((VedApp*)wrenGetUserData(vm))->next_buffer();
}

static void slate_prev_buffer(WrenVM* vm) {
    ((VedApp*)wrenGetUserData(vm))->prev_buffer();
}

static void slate_new_buffer(WrenVM* vm) {
    VedApp* app = (VedApp*)wrenGetUserData(vm);
    app->new_buffer_named(wrenGetSlotString(vm, 1));
}

// ── Keybinds & commands ───────────────────────────────────────────────────────

static void slate_bind_command(WrenVM* vm) {
    VedApp* app = (VedApp*)wrenGetUserData(vm);
    const char* name = wrenGetSlotString(vm, 1);
    WrenHandle* receiver = wrenGetSlotHandle(vm, 2);
    WrenHandle* method   = wrenMakeCallHandle(vm, "call(_)");
    app->bind_wren_command(name, receiver, method);
}

static void slate_bind_key(WrenVM* vm) {
    VedApp* app = (VedApp*)wrenGetUserData(vm);
    const char* key     = wrenGetSlotString(vm, 1);
    WrenHandle* receiver = wrenGetSlotHandle(vm, 2);
    WrenHandle* method   = wrenMakeCallHandle(vm, "call(_,_)");
    app->bind_wren_key(key, receiver, method);
}

static void slate_bind_insert_key(WrenVM* vm) {
    VedApp* app = (VedApp*)wrenGetUserData(vm);
    const char* key     = wrenGetSlotString(vm, 1);
    WrenHandle* receiver = wrenGetSlotHandle(vm, 2);
    WrenHandle* method   = wrenMakeCallHandle(vm, "call(_)");
    app->bind_wren_insert_key(key, receiver, method);
}

static void slate_bind_on_change(WrenVM* vm) {
    VedApp* app = (VedApp*)wrenGetUserData(vm);
    WrenHandle* receiver = wrenGetSlotHandle(vm, 1);
    WrenHandle* method   = wrenMakeCallHandle(vm, "call()");
    app->bind_wren_on_change(receiver, method);
}

static void slate_bind_on_save(WrenVM* vm) {
    VedApp* app = (VedApp*)wrenGetUserData(vm);
    WrenHandle* receiver = wrenGetSlotHandle(vm, 1);
    WrenHandle* method   = wrenMakeCallHandle(vm, "call()");
    app->bind_wren_on_save(receiver, method);
}

static void slate_bind_on_open(WrenVM* vm) {
    VedApp* app = (VedApp*)wrenGetUserData(vm);
    WrenHandle* receiver = wrenGetSlotHandle(vm, 1);
    WrenHandle* method   = wrenMakeCallHandle(vm, "call()");
    app->bind_wren_on_open(receiver, method);
}

static void slate_bind_on_mode_change(WrenVM* vm) {
    VedApp* app = (VedApp*)wrenGetUserData(vm);
    WrenHandle* receiver = wrenGetSlotHandle(vm, 1);
    WrenHandle* method   = wrenMakeCallHandle(vm, "call(_,_)");
    app->bind_wren_on_mode_change(receiver, method);
}

// ── UI ────────────────────────────────────────────────────────────────────────

static void slate_add_status_seg(WrenVM* vm) {
    VedApp* app = (VedApp*)wrenGetUserData(vm);
    WrenHandle* receiver = wrenGetSlotHandle(vm, 1);
    WrenHandle* method   = wrenMakeCallHandle(vm, "call()");
    app->add_wren_status_seg(receiver, method);
}

static void slate_show_overlay(WrenVM* vm) {
    VedApp* app = (VedApp*)wrenGetUserData(vm);
    if (wrenGetSlotType(vm, 1) == WREN_TYPE_NULL ||
        wrenGetSlotType(vm, 1) == WREN_TYPE_BOOL) {
        app->set_overlay("");
    } else {
        app->set_overlay(wrenGetSlotString(vm, 1));
    }
}

// ── Syntax highlighting ───────────────────────────────────────────────────────

static void slate_add_highlight_rule(WrenVM* vm) {
    VedApp* app = (VedApp*)wrenGetUserData(vm);
    const char* ext   = wrenGetSlotString(vm, 1);
    const char* pat   = wrenGetSlotString(vm, 2);
    const char* type  = wrenGetSlotString(vm, 3);
    app->add_highlight_rule(ext, pat, type);
}

static void slate_clear_highlight_rules(WrenVM* vm) {
    VedApp* app = (VedApp*)wrenGetUserData(vm);
    app->clear_highlight_rules(wrenGetSlotString(vm, 1));
}

// ════════════════════════════════════════════════════════════════════════════
//  bind_method dispatch
// ════════════════════════════════════════════════════════════════════════════

/*static*/ WrenForeignMethodFn ScriptingEngine::bind_method(
    WrenVM*, const char*, const char* class_name, bool, const char* sig)
{
    if (std::string(class_name) != "Slate") return nullptr;
    std::string s(sig);

    // ── Existing ──────────────────────────────────────────────────────────
    if (s == "apiVersion()")           return slate_api_version;
    if (s == "openFile(_)")            return slate_open_file;
    if (s == "setStatus(_)")           return slate_set_status;
    if (s == "closeAll()")             return slate_close_all;
    if (s == "bufferName()")           return slate_buffer_name;
    if (s == "getLine(_)")             return slate_get_line;

    // ── Buffer access ──────────────────────────────────────────────────────
    if (s == "lineCount()")            return slate_line_count;
    if (s == "setLine(_,_)")           return slate_set_line;
    if (s == "insertLine(_,_)")        return slate_insert_line;
    if (s == "deleteLine(_)")          return slate_delete_line;
    if (s == "getCursor()")            return slate_get_cursor;
    if (s == "setCursor(_,_)")         return slate_set_cursor;
    if (s == "getYankRegister()")      return slate_get_yank;
    if (s == "setYankRegister(_)")     return slate_set_yank;

    // ── Editor state ───────────────────────────────────────────────────────
    if (s == "getMode()")              return slate_get_mode;
    if (s == "setMode(_)")             return slate_set_mode;
    if (s == "getSearchQuery()")       return slate_get_search;
    if (s == "setSearchQuery(_)")      return slate_set_search;
    if (s == "isModified()")           return slate_is_modified;
    if (s == "filePath()")             return slate_file_path;

    // ── Undo / Redo ────────────────────────────────────────────────────────
    if (s == "undo()")                 return slate_undo;
    if (s == "redo()")                 return slate_redo;
    if (s == "pushUndo()")             return slate_push_undo;

    // ── Actions ────────────────────────────────────────────────────────────
    if (s == "exec(_)")                return slate_exec;
    if (s == "saveFile()")             return slate_save_file;
    if (s == "closeBuffer()")          return slate_close_buffer;
    if (s == "nextBuffer()")           return slate_next_buffer;
    if (s == "prevBuffer()")           return slate_prev_buffer;
    if (s == "newBuffer(_)")           return slate_new_buffer;

    // ── Keybinds & commands ────────────────────────────────────────────────
    if (s == "bindCommand(_,_)")       return slate_bind_command;
    if (s == "bindKey(_,_)")           return slate_bind_key;
    if (s == "bindInsertKey(_,_)")     return slate_bind_insert_key;
    if (s == "bindOnChange(_)")        return slate_bind_on_change;
    if (s == "bindOnSave(_)")          return slate_bind_on_save;
    if (s == "bindOnOpen(_)")          return slate_bind_on_open;
    if (s == "bindOnModeChange(_)")    return slate_bind_on_mode_change;

    // ── UI ─────────────────────────────────────────────────────────────────
    if (s == "addStatusSegment(_)")    return slate_add_status_seg;
    if (s == "showOverlay(_)")         return slate_show_overlay;

    // ── Syntax ─────────────────────────────────────────────────────────────
    if (s == "addHighlightRule(_,_,_)") return slate_add_highlight_rule;
    if (s == "clearHighlightRules(_)")  return slate_clear_highlight_rules;

    return nullptr;
}

// ════════════════════════════════════════════════════════════════════════════
//  VM lifecycle
// ════════════════════════════════════════════════════════════════════════════

/*static*/ void ScriptingEngine::write(WrenVM*, const char* text) {
    std::cerr << text;
}

/*static*/ void ScriptingEngine::error_handler(WrenVM* vm, WrenErrorType type,
    const char* module, int line, const char* msg)
{
    VedApp* app = (VedApp*)wrenGetUserData(vm);
    std::string err;
    if (type == WREN_ERROR_COMPILE)
        err = std::string(module) + ":" + std::to_string(line) + ": " + msg;
    else if (type == WREN_ERROR_RUNTIME)
        err = std::string("runtime: ") + msg;
    else
        err = std::string("stack: ") + msg;
    app->set_status(err);
    std::cerr << err << '\n';
}

ScriptingEngine::ScriptingEngine(VedApp& app) : app_(app) { init_vm(); }
ScriptingEngine::~ScriptingEngine() { if (vm_) wrenFreeVM(vm_); }

void ScriptingEngine::init_vm() {
    WrenConfiguration cfg;
    wrenInitConfiguration(&cfg);
    cfg.writeFn             = &ScriptingEngine::write;
    cfg.errorFn             = &ScriptingEngine::error_handler;
    cfg.bindForeignMethodFn = &ScriptingEngine::bind_method;
    cfg.userData            = &app_;
    vm_ = wrenNewVM(&cfg);

    // ── Wren bootstrap ────────────────────────────────────────────────────
    const char* bootstrap = R"(
class Slate {
    // meta
    foreign static apiVersion()

    // buffer access
    foreign static lineCount()
    foreign static getLine(n)
    foreign static setLine(n, str)
    foreign static insertLine(n, str)
    foreign static deleteLine(n)
    foreign static getCursor()
    foreign static setCursor(row, col)
    foreign static getYankRegister()
    foreign static setYankRegister(str)

    // editor state
    foreign static getMode()
    foreign static setMode(str)
    foreign static getSearchQuery()
    foreign static setSearchQuery(str)
    foreign static isModified()
    foreign static filePath()
    foreign static bufferName()

    // undo/redo
    foreign static undo()
    foreign static redo()
    foreign static pushUndo()

    // actions
    foreign static exec(cmd)
    foreign static openFile(path)
    foreign static saveFile()
    foreign static closeBuffer()
    foreign static closeAll()
    foreign static nextBuffer()
    foreign static prevBuffer()
    foreign static newBuffer(name)

    // keybinds & commands
    foreign static bindCommand(name, fn)
    foreign static bindKey(key, fn)
    foreign static bindInsertKey(key, fn)
    foreign static bindOnChange(fn)
    foreign static bindOnSave(fn)
    foreign static bindOnOpen(fn)
    foreign static bindOnModeChange(fn)

    // UI
    foreign static setStatus(msg)
    foreign static addStatusSegment(fn)
    foreign static showOverlay(str)

    // syntax
    foreign static addHighlightRule(ext, pattern, tokenType)
    foreign static clearHighlightRules(ext)
}
)";
    wrenInterpret(vm_, "slate", bootstrap);
}

// ════════════════════════════════════════════════════════════════════════════
//  Call helpers
// ════════════════════════════════════════════════════════════════════════════

void ScriptingEngine::call(WrenCallback& cb, const std::string& arg) {
    if (!cb.valid()) return;
    wrenEnsureSlots(vm_, 2);
    wrenSetSlotHandle(vm_, 0, cb.receiver);
    wrenSetSlotString(vm_, 1, arg.c_str());
    wrenCall(vm_, cb.method);
}

void ScriptingEngine::call0(WrenCallback& cb) {
    if (!cb.valid()) return;
    wrenEnsureSlots(vm_, 1);
    wrenSetSlotHandle(vm_, 0, cb.receiver);
    wrenCall(vm_, cb.method);
}

void ScriptingEngine::call2(WrenCallback& cb,
                            const std::string& a, const std::string& b) {
    if (!cb.valid()) return;
    wrenEnsureSlots(vm_, 3);
    wrenSetSlotHandle(vm_, 0, cb.receiver);
    wrenSetSlotString(vm_, 1, a.c_str());
    wrenSetSlotString(vm_, 2, b.c_str());
    wrenCall(vm_, cb.method);
}

std::string ScriptingEngine::call_str(WrenCallback& cb) {
    if (!cb.valid()) return "";
    wrenEnsureSlots(vm_, 1);
    wrenSetSlotHandle(vm_, 0, cb.receiver);
    wrenCall(vm_, cb.method);
    if (wrenGetSlotType(vm_, 0) == WREN_TYPE_STRING)
        return wrenGetSlotString(vm_, 0);
    return "";
}

// ════════════════════════════════════════════════════════════════════════════
//  File loading
// ════════════════════════════════════════════════════════════════════════════

void ScriptingEngine::load_file(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) { app_.set_status("script not found: " + path); return; }
    std::ostringstream ss; ss << f.rdbuf();
    if (wrenInterpret(vm_, path.c_str(), ss.str().c_str()) != WREN_RESULT_SUCCESS)
        app_.set_status("script error: " + path);
}

void ScriptingEngine::load_plugins_dir(const std::string& dir) {
    if (!fs::exists(dir)) return;
    for (auto& entry : fs::directory_iterator(dir)) {
        if (!entry.is_directory()) continue;
        auto main = entry.path() / "main.wren";
        if (fs::exists(main)) load_file(main.string());
    }
}

void ScriptingEngine::error(const std::string& msg) { app_.set_status(msg); }
