#include "meson_runner.h"
void MesonRunner::configure(const std::string&, const std::string&, const std::vector<std::string>&, DoneCb done) { if(done) done(false); }
void MesonRunner::build(const std::string&, OutputCb, DoneCb done) { if(done) done(false); }
void MesonRunner::clean(const std::string&, DoneCb done) { if(done) done(false); }
void MesonRunner::run_tests(const std::string&, const std::string&, OutputCb, std::function<void(std::vector<TestResult>)> done) { if(done) done({}); }
void MesonRunner::wrap_list(std::function<void(std::vector<WrapEntry>)> cb) { if(cb) cb({}); }
void MesonRunner::wrap_install(const std::string&, const std::string&, DoneCb done) { if(done) done(false); }
void MesonRunner::wrap_update(const std::string&, DoneCb done) { if(done) done(false); }
bool MesonRunner::create_project(const std::string&, const std::string&, const std::string&, const std::string&) { return false; }
void MesonRunner::cancel() {}
