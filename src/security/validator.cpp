#include "security/validator.h"
#include <cctype>

namespace hms {

namespace {
bool noControlChars(const std::string& s) {
    for (unsigned char c : s) if (c < 0x20 || c == 0x7F) return false;
    return true;
}
bool allDigits(const std::string& s) {
    for (unsigned char c : s) if (!std::isdigit(c)) return false;
    return true;
}
}  // namespace

bool Validator::isValidLineLength(const std::string& s) { return !s.empty() && s.size() <= kMaxLine && noControlChars(s); }

bool Validator::isValidUsername(const std::string& s) {
    if (s.size() < 4 || s.size() > 20) return false;
    for (unsigned char c : s) if (!std::isalnum(c)) return false;
    return true;
}

bool Validator::isValidPassword(const std::string& s) {
    if (s.size() < 8 || s.size() > 32 || !noControlChars(s)) return false;
    bool letter = false, digit = false;
    for (unsigned char c : s) { letter |= std::isalpha(c) != 0; digit |= std::isdigit(c) != 0; }
    return letter && digit;
}

bool Validator::isValidPatientName(const std::string& s) {
    if (s.empty() || s.size() > 60) return false;
    for (unsigned char c : s) if (!(std::isalpha(c) || c == ' ' || c == '.' || c == '-')) return false;
    return true;
}

bool Validator::isValidPhone(const std::string& s) { return s.size() == 10 && allDigits(s); }

bool Validator::isValidDate(const std::string& s) {
    if (s.size() != 10 || s[4] != '-' || s[7] != '-') return false;
    if (!allDigits(s.substr(0, 4)) || !allDigits(s.substr(5, 2)) || !allDigits(s.substr(8, 2))) return false;
    int y = std::stoi(s.substr(0, 4)), m = std::stoi(s.substr(5, 2)), d = std::stoi(s.substr(8, 2));
    if (y < 1900 || m < 1 || m > 12 || d < 1) return false;
    static const int dm[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    int maxd = dm[m - 1];
    if (m == 2 && ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0)) maxd = 29;
    return d <= maxd;
}

bool Validator::isDateNotAfter(const std::string& date, const std::string& today) { return date <= today; }

bool Validator::isValidGender(const std::string& s) { return s == "M" || s == "F" || s == "O"; }

bool Validator::isValidBloodGroup(const std::string& s) {
    static const char* ok[] = {"A+","A-","B+","B-","AB+","AB-","O+","O-"};
    for (auto* g : ok) if (s == g) return true;
    return false;
}

bool Validator::isValidText(const std::string& s, size_t maxLen) { return !s.empty() && s.size() <= maxLen && noControlChars(s); }
}  // namespace hms
