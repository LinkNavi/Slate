#include "gdb_client.h"
GDBClient::GDBClient()  = default;
GDBClient::~GDBClient() { stop(); }
bool GDBClient::start(const std::string&) { return false; }
void GDBClient::stop()         {}
void GDBClient::run()          {}
void GDBClient::cont()         {}
void GDBClient::step_over()    {}
void GDBClient::step_into()    {}
void GDBClient::step_out()     {}
void GDBClient::interrupt()    {}
int  GDBClient::set_breakpoint(const std::string&, int) { return -1; }
void GDBClient::remove_breakpoint(int) {}
void GDBClient::toggle_breakpoint(int, bool) {}
void GDBClient::get_stack(std::function<void(std::vector<StackFrame>)>) {}
void GDBClient::get_locals(std::function<void(std::vector<Variable>)>) {}
void GDBClient::evaluate(const std::string&, std::function<void(std::string)>) {}
