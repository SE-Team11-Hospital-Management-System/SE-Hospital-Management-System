#include "security/session_manager.h"
#include "security/sha256.h"
#include <random>

namespace hms {

SessionManager::SessionManager(Clock clock, std::chrono::minutes idleLimit)
    : clock_(std::move(clock)), idleLimit_(idleLimit) {}

Session SessionManager::create(const std::string& userId, Role role, const std::string& doctorId) {
    Session s;
    std::random_device rd;                                         // unguessable token: 128 random bits + counter
    std::string seed = userId + std::to_string(++counter_);
    for (int i = 0; i < 4; ++i) seed += std::to_string(rd());
    s.token = sha256Hex(seed);
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

void SessionManager::attachDoctor(const std::string& token, const std::string& doctorId) {
    auto it = sessions_.find(token);
    if (it != sessions_.end()) it->second.doctorId = doctorId;
}

void SessionManager::end(const std::string& token) { sessions_.erase(token); }
}  // namespace hms
