#include "ui/console_ui.h"
#include "security/validator.h"
#include <cctype>
#include <iostream>
#ifdef _WIN32
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

namespace hms {

namespace {
bool g_closed = false;
std::string trim(const std::string& s) {
    size_t a = 0, b = s.size();
    while (a < b && std::isspace(static_cast<unsigned char>(s[a]))) ++a;
    while (b > a && std::isspace(static_cast<unsigned char>(s[b - 1]))) --b;
    return s.substr(a, b - a);
}
}  // namespace

bool ConsoleUi::inputClosed() { return g_closed; }

std::string ConsoleUi::readLine(const std::string& prompt) {
    std::cout << prompt << std::flush;
    std::string s;
    if (!std::getline(std::cin, s)) { g_closed = true; return ""; }
    if (!s.empty() && s.back() == '\r') s.pop_back();
    if (s.size() > Validator::kMaxLine) s.resize(Validator::kMaxLine);   // bounded input (HMS-SEC-005)
    return trim(s);
}

std::string ConsoleUi::readPassword(const std::string& prompt) {
    std::cout << prompt << std::flush;
    std::string s;
#ifdef _WIN32
    int c;
    while ((c = _getch()) != '\r' && c != '\n' && c != EOF) {
        if (c == '\b') { if (!s.empty()) s.pop_back(); }
        else if (s.size() < Validator::kMaxLine) s += static_cast<char>(c);
    }
#else
    termios oldt{};
    const bool tty = isatty(STDIN_FILENO) && tcgetattr(STDIN_FILENO, &oldt) == 0;
    if (tty) { termios t = oldt; t.c_lflag &= ~static_cast<tcflag_t>(ECHO); tcsetattr(STDIN_FILENO, TCSANOW, &t); }
    if (!std::getline(std::cin, s)) g_closed = true;
    if (tty) tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    if (!s.empty() && s.back() == '\r') s.pop_back();
    if (s.size() > Validator::kMaxLine) s.resize(Validator::kMaxLine);
#endif
    std::cout << '\n';
    return s;
}

int ConsoleUi::readChoice(const std::string& prompt) {
    const std::string s = readLine(prompt);
    if (s.empty() || s.size() > 3) return -1;
    for (unsigned char c : s) if (!std::isdigit(c)) return -1;
    return std::stoi(s);
}

bool ConsoleUi::confirm(const std::string& prompt) {
    const std::string s = readLine(prompt + " (Y/N): ");
    return s == "Y" || s == "y";
}

void ConsoleUi::printError(const Error& e) { printError(codeName(e.code), e.message); }
void ConsoleUi::printError(const std::string& code, const std::string& message) { std::cout << "ERROR " << code << ": " << message << '\n'; }
void ConsoleUi::printOk(const std::string& message) { std::cout << "OK: " << message << '\n'; }
void ConsoleUi::header(const std::string& title) { std::cout << "\n==== " << title << " ====\n"; }
}  // namespace hms
