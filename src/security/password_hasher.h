// C11 PasswordHasher: salted, iterated SHA-256 (HMS-SEC-001, ADR-6).
#pragma once
#include <string>
namespace hms {
class PasswordHasher {
public:
    static std::string generateSalt();                                   // 16 random bytes -> 32 hex chars
    static std::string hash(const std::string& password, const std::string& salt);
    static bool verify(const std::string& password, const std::string& salt, const std::string& storedHash);
    static constexpr int kIterations = 10000;
};
}  // namespace hms
