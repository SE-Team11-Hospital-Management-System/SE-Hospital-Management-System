// C1 Console UI helpers: bounded line input (HMS-SEC-005), silent password entry (HMS-SEC-004), common messages.
#pragma once
#include "common/types.h"
#include <string>
namespace hms {
class ConsoleUi {
public:
    static std::string readLine(const std::string& prompt);        // never returns more than Validator::kMaxLine chars
    static std::string readPassword(const std::string& prompt);    // characters are not echoed
    static int readChoice(const std::string& prompt);              // -1 if not a number
    static bool confirm(const std::string& prompt);                // true only for Y / y
    static void printError(const Error& e);                        // "ERROR E010: Phone: must be exactly 10 digits."
    static void printError(const std::string& code, const std::string& message);
    static void printOk(const std::string& message);
    static void header(const std::string& title);
    static bool inputClosed();                                     // true after end of input (Ctrl+D / Ctrl+Z)
};
}  // namespace hms
