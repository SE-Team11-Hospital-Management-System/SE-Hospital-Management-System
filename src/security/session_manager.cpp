// SKELETON STUB (TASK-02). Replaced by the owner's pull request during Sprint 1 - see the header for the owner.
// Owner: Member 1 (Paras).
#include "security/session_manager.h"
namespace hms {
SessionManager::SessionManager(Clock clock, std::chrono::minutes idleLimit) : clock_(std::move(clock)), idleLimit_(idleLimit) {}
Session SessionManager::create(const std::string& userId, Role role, const std::string& doctorId) {
    Session s; s.token = userId + "-" + std::to_string(++counter_); s.userId = userId; s.role = role; s.doctorId = doctorId;
    return s;
}
Result<Session> SessionManager::validate(const std::string&) { return notImplementedError(); }
void SessionManager::end(const std::string&) {}
}  // namespace hms
