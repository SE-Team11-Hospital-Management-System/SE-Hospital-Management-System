#include "data/file_store.h"
#include "security/sha256.h"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace hms {

namespace {
void ownerOnly(const fs::path& p, bool dir) {                    // HMS-SEC-006: 700 for folders, 600 for files
    std::error_code ec;
    fs::permissions(p, dir ? fs::perms::owner_all : (fs::perms::owner_read | fs::perms::owner_write),
                    fs::perm_options::replace, ec);              // on Windows the user-profile ACL already limits access
}
}  // namespace

std::string FileStore::checksumLine(const std::vector<std::string>& lines) {
    std::string all;
    for (const auto& l : lines) { all += l; all += '\n'; }
    return "#SHA256=" + sha256Hex(all);
}

bool FileStore::exists(const std::string& path) { std::error_code ec; return fs::exists(path, ec); }

Result<void> FileStore::ensureDirectory(const std::string& dir) {
    std::error_code ec;
    fs::create_directories(dir, ec);
    if (ec || !fs::is_directory(dir, ec)) return writeFailedError();
    ownerOnly(dir, true);
    return {};
}

Result<void> FileStore::atomicWrite(const std::string& path, const std::vector<std::string>& lines) {
    const std::string tmp = path + ".tmp";
    std::error_code ec;
    {
        std::ofstream out(tmp, std::ios::trunc | std::ios::binary);
        if (!out) return writeFailedError();                         // HMS-NF-011: nothing changed
        ownerOnly(tmp, false);
        for (const auto& l : lines) out << l << '\n';
        out << checksumLine(lines) << '\n';                          // HMS-SEC-007
        out.flush();
        if (!out) { out.close(); fs::remove(tmp, ec); return writeFailedError(); }
    }
    fs::rename(tmp, path, ec);                                       // HMS-NF-005: old file or new file, never half a file
    if (ec) { fs::remove(tmp, ec); return writeFailedError(); }
    ownerOnly(path, false);
    return {};
}

Result<std::vector<std::string>> FileStore::load(const std::string& path) {
    std::vector<std::string> lines;
    std::error_code ec;
    if (!fs::exists(path, ec)) return lines;                         // first run: no data yet
    std::ifstream in(path, std::ios::binary);
    if (!in) return makeError(ErrorCode::E040, "Data file damaged. Restore from backup?", path);
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (!line.empty()) lines.push_back(line);
    }
    if (lines.empty() || lines.back().rfind("#SHA256=", 0) != 0)            // HMS-SEC-007: no checksum = not trusted
        return makeError(ErrorCode::E040, "Data file damaged. Restore from backup?", path);
    const std::string stored = lines.back();
    lines.pop_back();
    if (stored != checksumLine(lines)) return makeError(ErrorCode::E040, "Data file damaged. Restore from backup?", path);
    return lines;
}
}  // namespace hms
