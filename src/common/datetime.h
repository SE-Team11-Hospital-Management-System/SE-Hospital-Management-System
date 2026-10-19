// Helpers for "YYYY-MM-DD HH:MM" strings (ISO order, so string comparison == time comparison).
#pragma once
#include <ctime>
#include <string>
namespace hms {
namespace dt {

inline bool parse(const std::string& s, std::tm& out) {
    if (s.size() != 16 || s[4] != '-' || s[7] != '-' || s[10] != ' ' || s[13] != ':') return false;
    for (size_t i : {0u, 1u, 2u, 3u, 5u, 6u, 8u, 9u, 11u, 12u, 14u, 15u})
        if (s[i] < '0' || s[i] > '9') return false;
    out = {};
    out.tm_year = std::stoi(s.substr(0, 4)) - 1900; out.tm_mon = std::stoi(s.substr(5, 2)) - 1;
    out.tm_mday = std::stoi(s.substr(8, 2));        out.tm_hour = std::stoi(s.substr(11, 2));
    out.tm_min = std::stoi(s.substr(14, 2));
    out.tm_isdst = -1;
    return out.tm_mon >= 0 && out.tm_mon < 12 && out.tm_mday >= 1 && out.tm_mday <= 31 && out.tm_hour < 24 && out.tm_min < 60;
}
inline std::string format(std::tm t) {
    std::mktime(&t);
    char b[20]; std::strftime(b, sizeof b, "%Y-%m-%d %H:%M", &t);
    return b;
}
inline bool isValid(const std::string& s) { std::tm t; return parse(s, t) && format(t) == s; }   // rejects 2026-02-30 etc.
inline int weekday(const std::string& s) { std::tm t; if (!parse(s, t)) return -1; std::mktime(&t); return t.tm_wday; }   // 0 = Sunday
inline std::string addMinutes(const std::string& s, int minutes) { std::tm t; if (!parse(s, t)) return s; t.tm_min += minutes; return format(t); }
inline std::string timeOf(const std::string& s) { return s.size() == 16 ? s.substr(11, 5) : ""; }
inline std::string dateOf(const std::string& s) { return s.size() >= 10 ? s.substr(0, 10) : ""; }
inline bool onHalfHour(const std::string& s) { return s.size() == 16 && (s.substr(14, 2) == "00" || s.substr(14, 2) == "30"); }

inline std::tm localNow() {
    std::time_t t = std::time(nullptr); std::tm tmv{};
#ifdef _WIN32
    localtime_s(&tmv, &t);
#else
    localtime_r(&t, &tmv);
#endif
    return tmv;
}
inline std::string now() { std::tm t = localNow(); char b[20]; std::strftime(b, sizeof b, "%Y-%m-%d %H:%M", &t); return b; }
inline std::string today() { return now().substr(0, 10); }
inline std::string stamp() { std::tm t = localNow(); char b[24]; std::strftime(b, sizeof b, "%Y%m%d_%H%M%S", &t); return b; }
inline std::string isoNow() { std::tm t = localNow(); char b[24]; std::strftime(b, sizeof b, "%Y-%m-%dT%H:%M:%S", &t); return b; }
}  // namespace dt
}  // namespace hms
