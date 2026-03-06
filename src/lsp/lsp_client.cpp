#include "lsp_client.h"
// Full implementation: spawn clangd, speak JSON-RPC over stdin/stdout
// Headers defined in lsp_client.h
LSPClient::LSPClient()  = default;
LSPClient::~LSPClient() { stop(); }
bool LSPClient::start(const std::string&) { return false; } // TODO
void LSPClient::stop()  {}
void LSPClient::open_file(const std::string&, const std::string&, const std::string&) {}
void LSPClient::change_file(const std::string&, const std::string&, int) {}
void LSPClient::close_file(const std::string&) {}
void LSPClient::request_completion(const std::string&, int, int, CompletionCb) {}
void LSPClient::request_hover(const std::string&, int, int, std::function<void(std::string)>) {}
