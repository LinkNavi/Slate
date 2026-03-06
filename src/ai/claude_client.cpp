#include "claude_client.h"
void AIClient::complete(const std::string&, int, int, const std::string&, StreamCb, DoneCb done) { if(done) done("", false); }
void AIClient::explain(const std::string&, const std::string&, AIMode, StreamCb, DoneCb done)    { if(done) done("", false); }
void AIClient::diagnose(const std::string&, const std::string&, StreamCb, DoneCb done)           { if(done) done("", false); }
void AIClient::cancel() {}
