// Common types shared by all components (SAD 4.1 and 4.3 "Common types").
#pragma once
#include <string>
#include <utility>
#include <variant>

namespace hms {

enum class Role { Admin, Receptionist, Doctor };

inline const char* roleName(Role r) {
    switch (r) {
        case Role::Admin: return "Administrator";
        case Role::Receptionist: return "Receptionist";
        case Role::Doctor: return "Doctor";
    }
    return "?";
}

// Error catalogue: SAD section 4.4
enum class ErrorCode {
    OK, E001, E002, E003, E004, E010, E011, E020, E021, E022, E023, E024, E025,
    E030, E031, E032, E040, E041, E042, E043, E099
};

inline const char* codeName(ErrorCode c) {
    static const char* names[] = {"OK", "E001", "E002", "E003", "E004", "E010", "E011", "E020", "E021", "E022", "E023",
                                  "E024", "E025", "E030", "E031", "E032", "E040", "E041", "E042", "E043", "E099"};
    return names[static_cast<int>(c)];
}

struct Error {
    ErrorCode code{ErrorCode::OK};
    std::string message;
    std::string details;
};

// Result<T>: either a value or an Error (ADR-7). No exceptions cross layer boundaries.
template <class T>
class Result {
public:
    Result(T value) : v_(std::move(value)) {}
    Result(Error e) : v_(std::move(e)) {}
    bool ok() const { return std::holds_alternative<T>(v_); }
    const T& value() const { return std::get<T>(v_); }
    const Error& error() const { return std::get<Error>(v_); }
private:
    std::variant<T, Error> v_;
};

template <>
class Result<void> {
public:
    Result() = default;
    Result(Error e) : err_(std::move(e)), ok_(false) {}
    bool ok() const { return ok_; }
    const Error& error() const { return err_; }
private:
    Error err_;
    bool ok_{true};
};

inline Error makeError(ErrorCode c, std::string msg, std::string details = "") {
    return Error{c, std::move(msg), std::move(details)};
}

inline Error deniedError() { return makeError(ErrorCode::E004, "You are not authorised to perform this action."); }
inline Error writeFailedError() { return makeError(ErrorCode::E041, "Could not save data. Nothing was changed."); }
inline Error notImplementedError() { return makeError(ErrorCode::E099, "Not implemented yet (planned for this sprint)."); }

}  // namespace hms
