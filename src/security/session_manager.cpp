#include "security/session_manager.h"
#include "security/sha256.h"

namespace hms {

SessionManager::SessionManager(Clock clock, std::chrono::minutes idleLimit)
    : clock_(std::move(clock)), idleLimit_(idleLimit) {}

Session SessionManager::create(const std::string& userId, Role role, const std::string& doctorId) {
    Session s;
    s.token = sha256Hex(userId + std::to_string(++counter_) + std::to_string(clock_().time_since_epoch().count()));
    s.userId = userId; s.role = role; s.doctorId = doctorId; s.lastActivity = clock_();
    sessions_[s.token] = s;
    return s;
}

Result<Session> SessionManager::validate(const std::string& token) {
    auto it = sessions_.find(token);
    if (it == sessions_.end())
        return makeError(ErrorCode::E003, "Session expired. Please log in again.");
    if (clock_() - it->second.lastActivity > idleLimit_) {
        sessions_.erase(it);
        return makeError(ErrorCode::E003, "Session expired. Please log in again.");
    }
    it->second.lastActivity = clock_();
    return it->second;
}

void SessionManager::end(const std::string& token) { sessions_.erase(token); }
}  // namespace hms
