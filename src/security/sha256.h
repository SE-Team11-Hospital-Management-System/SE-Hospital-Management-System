// Self-contained SHA-256 (fallback when OpenSSL is unavailable, SAD risk R6).
#pragma once
#include <string>
namespace hms {
std::string sha256Hex(const std::string& data);   // 64 lower-case hex chars
}
