// C3 AuthService: login with lock-out, change password, user management (HMS-F-001/002/004/006, SEC-001/008).
// Users are saved to <dataDir>/users.dat, one per line:
//   userId|username|role(0=Admin,1=Receptionist,2=Doctor)|salt|passwordHash|active(1/0)|failedAttempts|lockedUntil(epoch seconds)
#pragma once
#include "common/types.h"
#include "security/audit_service.h"
#include "security/session_manager.h"
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace hms {

struct User {
    std::string userId, username, salt, passwordHash;
    Role role{Role::Receptionist};
    bool active{true};
    int failedAttempts{0};
    std::chrono::system_clock::time_point lockedUntil{};
};

class AuthService {
public:
    AuthService(SessionManager& sessions, AuditService& audit, std::string dataDir,
                Clock clock = [] { return std::chrono::system_clock::now(); });

    Result<void> load();                                                       // reads users.dat
    bool hasUsers() const { return !users_.empty(); }

    // First run only (SRS 2.6): creates the first Administrator when no user exists yet.
    Result<std::string> bootstrapAdmin(const std::string& username, const std::string& password);

    Result<Session> login(const std::string& username, const std::string& password);
    Result<void> logout(const Session& s);
    // HMS-F-006: current password must match; new one 8-32 chars, a letter and a digit, different from the current one.
    Result<void> changePassword(const Session& s, const std::string& currentPwd, const std::string& newPwd);
    Result<std::string> createUser(const Session& admin, const std::string& username, const std::string& tempPwd, Role role);
    Result<void> setUserStatus(const Session& admin, const std::string& userId, bool active);
    Result<std::vector<User>> listUsers(const Session& admin) const;
    std::optional<User> findUser(const std::string& userId) const;

    static constexpr int kMaxFailures = 3;
    static constexpr int kLockMinutes = 15;

private:
    User* findByUsername(const std::string& name);
    std::string nextUserId() const;
    Result<void> persist() const;
    SessionManager& sessions_;
    AuditService& audit_;
    std::string path_;
    Clock clock_;
    std::map<std::string, User> users_;      // userId -> User
};
}  // namespace hms
