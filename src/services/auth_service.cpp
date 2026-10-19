// SKELETON STUB (TASK-02). Replaced by the owner's pull request during Sprint 1 - see the header for the owner.
// Owner: Member 1 (Paras).
#include "services/auth_service.h"
namespace hms {
AuthService::AuthService(SessionManager& s, AuditService& a, std::string dataDir, Clock c)
    : sessions_(s), audit_(a), path_(dataDir + "/users.dat"), clock_(std::move(c)) {}
Result<void> AuthService::load() { return {}; }
Result<std::string> AuthService::bootstrapAdmin(const std::string&, const std::string&) { return notImplementedError(); }
Result<Session> AuthService::login(const std::string&, const std::string&) { return notImplementedError(); }
Result<void> AuthService::logout(const Session&) { return {}; }
Result<std::string> AuthService::createUser(const Session&, const std::string&, const std::string&, Role) { return notImplementedError(); }
Result<void> AuthService::setUserStatus(const Session&, const std::string&, bool) { return notImplementedError(); }
Result<std::vector<User>> AuthService::listUsers(const Session&) const { return notImplementedError(); }
std::optional<User> AuthService::findUser(const std::string& userId) const {
    auto it = users_.find(userId);
    if (it == users_.end()) return std::nullopt;
    return it->second;
}
}  // namespace hms
