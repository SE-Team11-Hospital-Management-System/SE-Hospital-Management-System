#include "services/auth_service.h"
#include "common/codec.h"
#include "data/file_store.h"
#include "security/access_control.h"
#include "security/password_hasher.h"
#include "security/validator.h"
#include <cstdio>
#include <cstdlib>

namespace hms {

namespace {
const char* kGenericLoginError = "Invalid username or password.";   // HMS-SEC-008: one message for every cause
long long toEpoch(std::chrono::system_clock::time_point t) { return std::chrono::duration_cast<std::chrono::seconds>(t.time_since_epoch()).count(); }
std::chrono::system_clock::time_point fromEpoch(long long s) { return std::chrono::system_clock::time_point(std::chrono::seconds(s)); }
}  // namespace

AuthService::AuthService(SessionManager& s, AuditService& a, std::string dataDir, Clock c)
    : sessions_(s), audit_(a), path_(dataDir + "/users.dat"), clock_(std::move(c)) {}

std::string AuthService::nextUserId() const {
    char id[16]; std::snprintf(id, sizeof id, "U%05d", static_cast<int>(users_.size()) + 1);
    return id;
}

User* AuthService::findByUsername(const std::string& name) {
    for (auto& [id, u] : users_) if (u.username == name) return &u;
    return nullptr;
}

std::optional<User> AuthService::findUser(const std::string& userId) const {
    auto it = users_.find(userId);
    if (it == users_.end()) return std::nullopt;
    return it->second;
}

Result<void> AuthService::load() {
    auto r = FileStore::load(path_);
    if (!r.ok()) return r.error();
    users_.clear();
    for (const auto& line : r.value()) {
        auto f = codec::split(line);
        if (f.size() != 8) return makeError(ErrorCode::E040, "Data file damaged. Restore from backup?", path_);
        User u;
        u.userId = f[0]; u.username = f[1]; u.role = static_cast<Role>(std::atoi(f[2].c_str()));
        u.salt = f[3]; u.passwordHash = f[4]; u.active = f[5] == "1";
        u.failedAttempts = std::atoi(f[6].c_str()); u.lockedUntil = fromEpoch(std::atoll(f[7].c_str()));
        users_[u.userId] = u;
    }
    return {};
}

Result<void> AuthService::persist() const {
    std::vector<std::string> lines;
    for (const auto& [id, u] : users_)                                 // only salt + hash are stored (HMS-SEC-001)
        lines.push_back(codec::join({u.userId, u.username, std::to_string(static_cast<int>(u.role)), u.salt, u.passwordHash,
                                     u.active ? "1" : "0", std::to_string(u.failedAttempts), std::to_string(toEpoch(u.lockedUntil))}));
    return FileStore::atomicWrite(path_, lines);
}

Result<std::string> AuthService::bootstrapAdmin(const std::string& username, const std::string& password) {
    if (!users_.empty()) return deniedError();
    if (!Validator::isValidUsername(username)) return makeError(ErrorCode::E010, "Username: 4-20 letters or digits.");
    if (!Validator::isValidPassword(password)) return makeError(ErrorCode::E010, "Password: 8-32 characters with at least one letter and one digit.");
    User u; u.userId = nextUserId(); u.username = username; u.role = Role::Admin;
    u.salt = PasswordHasher::generateSalt();
    u.passwordHash = PasswordHasher::hash(password, u.salt);
    users_[u.userId] = u;
    if (auto w = persist(); !w.ok()) { users_.erase(u.userId); return w.error(); }
    audit_.log(u.userId, "USER_CREATE", u.userId, true);
    return u.userId;
}

Result<Session> AuthService::login(const std::string& username, const std::string& password) {
    if (!Validator::isValidUsername(username) || !Validator::isValidLineLength(password)) {
        audit_.log("", "LOGIN_FAIL", "-", false);
        return makeError(ErrorCode::E001, kGenericLoginError);
    }
    User* u = findByUsername(username);
    if (!u || !u->active) {
        audit_.log("", "LOGIN_FAIL", u ? u->userId : "-", false);
        return makeError(ErrorCode::E001, kGenericLoginError);
    }
    const auto now = clock_();
    if (u->lockedUntil > now) {                                         // HMS-F-002: show remaining minutes
        const auto secs = std::chrono::duration_cast<std::chrono::seconds>(u->lockedUntil - now).count();
        const long long mins = (secs + 59) / 60;
        audit_.log(u->userId, "LOGIN_BLOCKED", u->userId, false);
        return makeError(ErrorCode::E002, "Account locked. Try again in " + std::to_string(mins) + " minutes.");
    }
    if (PasswordHasher::verify(password, u->salt, u->passwordHash)) {
        u->failedAttempts = 0;
        (void)persist();
        audit_.log(u->userId, "LOGIN_OK", u->userId, true);
        return sessions_.create(u->userId, u->role);
    }
    if (++u->failedAttempts >= kMaxFailures) {                          // HMS-F-002
        u->lockedUntil = now + std::chrono::minutes(kLockMinutes);
        u->failedAttempts = 0;
        audit_.log(u->userId, "ACCOUNT_LOCKED", u->userId, false);
    }
    (void)persist();                                                    // lock survives a restart
    audit_.log(u->userId, "LOGIN_FAIL", u->userId, false);
    return makeError(ErrorCode::E001, kGenericLoginError);
}

Result<void> AuthService::logout(const Session& s) {
    sessions_.end(s.token);
    audit_.log(s.userId, "LOGOUT", s.userId, true);
    return {};
}

Result<std::string> AuthService::createUser(const Session& admin, const std::string& username, const std::string& tempPwd, Role role) {
    if (auto ok = AccessControl::authorize(admin, Operation::ManageUsers, "createUser"); !ok.ok()) return ok.error();
    if (!Validator::isValidUsername(username)) return makeError(ErrorCode::E010, "Username: 4-20 letters or digits.");
    if (!Validator::isValidPassword(tempPwd)) return makeError(ErrorCode::E010, "Password: 8-32 characters with at least one letter and one digit.");
    if (findByUsername(username)) return makeError(ErrorCode::E011, "Username already exists.");
    User u; u.userId = nextUserId(); u.username = username; u.role = role;
    u.salt = PasswordHasher::generateSalt();
    u.passwordHash = PasswordHasher::hash(tempPwd, u.salt);
    users_[u.userId] = u;
    if (auto w = persist(); !w.ok()) { users_.erase(u.userId); return w.error(); }   // HMS-NF-011
    audit_.log(admin.userId, "USER_CREATE", u.userId, true);
    return u.userId;
}

Result<void> AuthService::setUserStatus(const Session& admin, const std::string& userId, bool active) {
    if (auto ok = AccessControl::authorize(admin, Operation::ManageUsers, "setUserStatus"); !ok.ok()) return ok.error();
    auto it = users_.find(userId);
    if (it == users_.end()) return makeError(ErrorCode::E023, "User not found.");
    if (userId == admin.userId) return makeError(ErrorCode::E010, "User: you cannot change the status of your own account.");
    const bool old = it->second.active;
    it->second.active = active;
    if (auto w = persist(); !w.ok()) { it->second.active = old; return w.error(); }
    audit_.log(admin.userId, active ? "USER_REACTIVATE" : "USER_DEACTIVATE", userId, true);
    return {};
}

Result<std::vector<User>> AuthService::listUsers(const Session& admin) const {
    if (auto ok = AccessControl::authorize(admin, Operation::ManageUsers, "listUsers"); !ok.ok()) return ok.error();
    std::vector<User> out;
    for (const auto& [id, u] : users_) out.push_back(u);
    return out;
}
Result<void> AuthService::changePassword(const Session& s, const std::string& currentPwd, const std::string& newPwd) {
    if (auto ok = AccessControl::authorize(s, Operation::ChangeOwnPassword, "changePassword"); !ok.ok()) return ok.error();
    auto it = users_.find(s.userId);
    if (it == users_.end()) return makeError(ErrorCode::E023, "User not found.");
    User& u = it->second;
    if (!PasswordHasher::verify(currentPwd, u.salt, u.passwordHash)) {
        audit_.log(s.userId, "PASSWORD_CHANGE", s.userId, false);
        return makeError(ErrorCode::E001, "Current password: incorrect.");
    }
    if (newPwd.size() < 8 || newPwd.size() > 32) return makeError(ErrorCode::E010, "New password: must have 8-32 characters.");
    if (!Validator::isValidPassword(newPwd)) return makeError(ErrorCode::E010, "New password: must contain at least one letter and one digit.");
    if (newPwd == currentPwd) return makeError(ErrorCode::E010, "New password: must differ from the current password.");
    const User before = u;
    u.salt = PasswordHasher::generateSalt();
    u.passwordHash = PasswordHasher::hash(newPwd, u.salt);
    if (auto w = persist(); !w.ok()) { u = before; return w.error(); }
    audit_.log(s.userId, "PASSWORD_CHANGE", s.userId, true);
    return {};
}
}  // namespace hms
