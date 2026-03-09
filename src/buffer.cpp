#include "buffer.h"
#include <fstream>
#include <sstream>

void Buffer::load(const std::string& path) {
    filepath = path;
    name = path.substr(path.find_last_of("/\\") + 1);
    lines.clear();
    std::ifstream f(path);
    if (!f.is_open()) { lines.push_back(""); return; }
    std::string line;
    while (std::getline(f, line)) lines.push_back(line);
    if (lines.empty()) lines.push_back("");
    modified = false;
}

void Buffer::save() {
    if (filepath.empty()) return;
    std::ofstream f(filepath);
    for (size_t i = 0; i < lines.size(); ++i) {
        f << lines[i];
        if (i + 1 < lines.size()) f << '\n';
    }
    modified = false;
}
