// C2 Input Validator: every user-supplied value is checked here before reaching a service (HMS-SEC-005, F-011).
#pragma once
#include <string>
namespace hms {

class Validator {
public:
    static constexpr size_t kMaxLine = 256;                 // HMS-SEC-005: no input line longer than 256 chars

    static bool isValidLineLength(const std::string& s);                  // 0 < len <= 256, no control chars / NUL
    static bool isValidUsername(const std::string& s);                    // 4-20 alphanumeric
    static bool isValidPassword(const std::string& s);                    // 8-32, >=1 letter, >=1 digit
    static bool isValidPatientName(const std::string& s);                 // 1-60, letters / space / . / -
    static bool isValidPhone(const std::string& s);                       // exactly 10 digits
    static bool isValidDate(const std::string& s);                        // YYYY-MM-DD, real calendar date
    static bool isDateNotAfter(const std::string& date, const std::string& today);   // lexicographic ISO compare
    static bool isValidGender(const std::string& s);                      // M / F / O
    static bool isValidBloodGroup(const std::string& s);                  // A+ A- B+ B- AB+ AB- O+ O-
    static bool isValidText(const std::string& s, size_t maxLen);         // non-control, <= maxLen
};
}  // namespace hms
