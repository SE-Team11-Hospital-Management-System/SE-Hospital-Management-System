// Part of C3: sessions with 10-minute idle time-out (HMS-F-005). Injectable clock for tests.
#pragma once
#include "common/types.h"
#include <chrono>
#include <functional>
#include <map>
#include <string>

namespace hms {
using Clock = std::function<std::chrono::system_clock::time_point()>;

struct Session {
    std::string token;
    std::string userId;
    Role role{Role::Receptionist};
    std::string doctorId;                       // empty unless role == Doctor
    std::chrono::system_clock::time_point lastActivity;
};

class SessionManager {
public:
    explicit SessionManager(Clock clock = [] { return std::chrono::system_clock::now(); },
                            std::chrono::minutes idleLimit = std::chrono::minutes(10));
    Session create(const std::string& userId, Role role, const std::string& doctorId = "");
    Result<Session> validate(const std::string& token);   // E003 if unknown / idle too long; refreshes activity
    void attachDoctor(const std::string& token, const std::string& doctorId);   // Doctor login -> own doctor profile
    void end(const std::string& token);
private:
    Clock clock_;
    std::chrono::minutes idleLimit_;
    std::map<std::string, Session> sessions_;
    unsigned long counter_{0};
};
}  // namespace hms
