// SKELETON STUB (TASK-02). Replaced by the owner's pull request during Sprint 1 - see the header for the owner.
// Owner: Member 4 (Shristi). BASIC VERSION: writes straight to the file (not atomic, no permissions yet).
#include "data/file_store.h"
#include <filesystem>
#include <fstream>
namespace fs = std::filesystem;
namespace hms {
bool FileStore::exists(const std::string& path) { std::error_code ec; return fs::exists(path, ec); }
Result<void> FileStore::ensureDirectory(const std::string& dir) {
    std::error_code ec; fs::create_directories(dir, ec);
    if (ec) return writeFailedError();
    return {};
}
Result<void> FileStore::atomicWrite(const std::string& path, const std::vector<std::string>& lines) {
    std::ofstream out(path, std::ios::trunc | std::ios::binary);
    if (!out) return writeFailedError();
    for (const auto& l : lines) out << l << '\n';
    return {};
}
Result<std::vector<std::string>> FileStore::load(const std::string& path) {
    std::vector<std::string> lines;
    std::ifstream in(path, std::ios::binary);
    std::string line;
    while (std::getline(in, line)) if (!line.empty()) lines.push_back(line);
    return lines;
}
}  // namespace hms
