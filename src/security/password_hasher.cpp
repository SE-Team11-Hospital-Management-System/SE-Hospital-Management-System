#include "security/password_hasher.h"
#include "security/sha256.h"
#include <cstdio>
#include <random>

namespace hms {

std::string PasswordHasher::generateSalt() {
    std::random_device rd;
    std::string out;
    char buf[3];
    for (int i = 0; i < 16; ++i) {
        std::snprintf(buf, sizeof buf, "%02x", static_cast<unsigned>(rd() & 0xFF));
        out += buf;
    }
    return out;
}

std::string PasswordHasher::hash(const std::string& password, const std::string& salt) {
    std::string digest = sha256Hex(salt + password);
    for (int i = 1; i < kIterations; ++i) digest = sha256Hex(digest + salt);
    return digest;
}

bool PasswordHasher::verify(const std::string& password, const std::string& salt, const std::string& storedHash) {
    const std::string h = hash(password, salt);
    if (h.size() != storedHash.size()) return false;
    unsigned char diff = 0;                         // constant-time comparison
    for (size_t i = 0; i < h.size(); ++i) diff |= static_cast<unsigned char>(h[i] ^ storedHash[i]);
    return diff == 0;
}
}  // namespace hms
