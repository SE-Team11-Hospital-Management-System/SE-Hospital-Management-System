#include "app/recovery.h"
#include "app/bootstrap.h"
#include "services/auth_service.h"
#include <filesystem>

namespace fs = std::filesystem;

namespace hms {

namespace {
// The backup is copied to a temporary folder first so that logging in never changes the backup.
Result<Session> adminLogin(AppContext& ctx, const std::string& backupDir, const std::string& user, const std::string& pwd) {
    std::error_code ec;
    const fs::path tmp = fs::temp_directory_path(ec) / "hms_recovery_check";
    fs::remove_all(tmp, ec);
    fs::create_directories(tmp, ec);
    fs::copy_file(fs::path(backupDir) / "users.dat", tmp / "users.dat", fs::copy_options::overwrite_existing, ec);
    AuthService check(ctx.sessions, ctx.audit, tmp.string());
    Result<Session> s = makeError(ErrorCode::E001, "Invalid username or password.");
    if (check.load().ok()) s = check.login(user, pwd);
    fs::remove_all(tmp, ec);
    if (s.ok() && s.value().role != Role::Admin) { ctx.sessions.end(s.value().token); return deniedError(); }
    return s;
}
}  // namespace

Result<std::string> restoreLatestBackup(AppContext& ctx, const std::string& username, const std::string& password) {
    const std::string latest = ctx.backup.latestBackup();
    if (latest.empty()) return makeError(ErrorCode::E042, "No backup is available.");
    auto admin = adminLogin(ctx, ctx.backup.backupRoot() + "/" + latest, username, password);
    if (!admin.ok()) return admin.error();
    auto r = ctx.backup.restore(admin.value(), latest, true);
    ctx.sessions.end(admin.value().token);
    if (!r.ok()) return r.error();
    auto again = loadAllData(ctx);
    if (!again.ok()) return makeError(ErrorCode::E040, "The backup " + latest + " is damaged too.", again.error().details);
    return latest;
}
}  // namespace hms
