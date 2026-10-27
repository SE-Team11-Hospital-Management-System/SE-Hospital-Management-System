#include "services/backup_service.h"
#include "common/datetime.h"
#include "data/file_store.h"
#include "security/access_control.h"
#include <algorithm>
#include <cctype>
#include <filesystem>

namespace fs = std::filesystem;

namespace hms {

namespace {
Error backupFailed() { return makeError(ErrorCode::E042, "Backup operation failed."); }
bool isBackupName(const std::string& n) {                                       // YYYYMMDD_HHMMSS (optional _N suffix)
    if (n.size() < 15 || n[8] != '_') return false;
    for (size_t i = 0; i < 15; ++i) if (i != 8 && !std::isdigit(static_cast<unsigned char>(n[i]))) return false;
    return n.find('/') == std::string::npos && n.find('\\') == std::string::npos && n.find("..") == std::string::npos;
}
void ownerOnlyFile(const fs::path& p) {
    std::error_code ec;
    fs::permissions(p, fs::perms::owner_read | fs::perms::owner_write, fs::perm_options::replace, ec);
}
}  // namespace

Result<std::string> BackupService::createBackup(const Session& s) {
    if (auto ok = AccessControl::authorize(s, Operation::Backup, "backup"); !ok.ok()) return ok.error();
    std::string name = dt::stamp();
    std::error_code ec;
    for (int i = 2; fs::exists(fs::path(backupRoot_) / name, ec); ++i) name = dt::stamp() + "_" + std::to_string(i);
    const fs::path dest = fs::path(backupRoot_) / name;
    if (!FileStore::ensureDirectory(backupRoot_).ok() || !FileStore::ensureDirectory(dest.string()).ok()) return backupFailed();
    for (const auto& e : fs::directory_iterator(dataDir_, ec)) {
        if (!e.is_regular_file() || e.path().extension() == ".tmp") continue;
        fs::copy_file(e.path(), dest / e.path().filename(), fs::copy_options::overwrite_existing, ec);
        if (ec) { audit_.log(s.userId, "BACKUP", name, false); return backupFailed(); }
        ownerOnlyFile(dest / e.path().filename());                              // HMS-SEC-006
    }
    if (ec) { audit_.log(s.userId, "BACKUP", name, false); return backupFailed(); }
    audit_.log(s.userId, "BACKUP", name, true);
    return name;
}

Result<std::vector<std::string>> BackupService::listBackups(const Session& s) const {
    if (auto ok = AccessControl::authorize(s, Operation::Backup, "listBackups"); !ok.ok()) return ok.error();
    std::vector<std::string> out; std::error_code ec;
    for (const auto& e : fs::directory_iterator(backupRoot_, ec))
        if (e.is_directory() && isBackupName(e.path().filename().string())) out.push_back(e.path().filename().string());
    std::sort(out.rbegin(), out.rend());
    return out;
}

std::string BackupService::latestBackup() const {
    std::vector<std::string> names; std::error_code ec;
    for (const auto& e : fs::directory_iterator(backupRoot_, ec))
        if (e.is_directory() && isBackupName(e.path().filename().string())) names.push_back(e.path().filename().string());
    return names.empty() ? "" : *std::max_element(names.begin(), names.end());
}

Result<void> BackupService::restore(const Session& s, const std::string& name, bool confirm) {
    if (auto ok = AccessControl::authorize(s, Operation::Backup, "restore"); !ok.ok()) return ok.error();
    if (!confirm) return makeError(ErrorCode::E010, "Restore: must be confirmed with Y.");
    if (!isBackupName(name)) return backupFailed();
    const fs::path src = fs::path(backupRoot_) / name;
    std::error_code ec;
    if (!fs::is_directory(src, ec)) return backupFailed();
    for (const auto& e : fs::directory_iterator(src, ec)) {
        if (!e.is_regular_file()) continue;
        if (e.path().filename() == "audit.log") continue;                      // the audit trail is never rolled back (HMS-SEC-003)
        // copy to .tmp then rename, so a failure never leaves a half-written data file (HMS-NF-005)
        const fs::path target = fs::path(dataDir_) / e.path().filename();
        const fs::path tmp = target.string() + ".tmp";
        fs::copy_file(e.path(), tmp, fs::copy_options::overwrite_existing, ec);
        if (!ec) fs::rename(tmp, target, ec);
        if (ec) { fs::remove(tmp, ec); audit_.log(s.userId, "RESTORE", name, false); return backupFailed(); }
        ownerOnlyFile(target);
    }
    audit_.log(s.userId, "RESTORE", name, true);
    return {};
}
}  // namespace hms
