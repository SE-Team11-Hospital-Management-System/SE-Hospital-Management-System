// Shared test helpers: a fresh temporary data folder per test and ready-made sessions.
#pragma once
#include "security/session_manager.h"
#include <filesystem>
#include <random>
#include <string>

namespace hms_test {
inline std::string freshDir(const std::string& tag) {
    std::random_device rd;
    auto p = std::filesystem::temp_directory_path() / ("hms_test_" + tag + "_" + std::to_string(rd()));
    std::filesystem::remove_all(p);
    std::filesystem::create_directories(p);
    return p.string();
}
inline hms::Session session(hms::Role r, const std::string& userId = "U00001") {
    hms::Session s; s.token = "t-" + userId; s.userId = userId; s.role = r; return s;
}
}  // namespace hms_test
