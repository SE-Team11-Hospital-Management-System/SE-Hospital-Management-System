// One record = one line, fields separated by '|'. '|' and '\' inside a field are escaped,
// so user text can never break the file format.
#pragma once
#include <string>
#include <vector>
namespace hms {
namespace codec {

inline std::string escape(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '\\') out += "\\\\";
        else if (c == '|') out += "\\p";
        else out += c;
    }
    return out;
}

inline std::string join(const std::vector<std::string>& fields) {
    std::string out;
    for (size_t i = 0; i < fields.size(); ++i) { if (i) out += '|'; out += escape(fields[i]); }
    return out;
}

inline std::vector<std::string> split(const std::string& line) {
    std::vector<std::string> out(1);
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (c == '\\' && i + 1 < line.size()) { char n = line[++i]; out.back() += (n == 'p' ? '|' : n); }
        else if (c == '|') out.emplace_back();
        else out.back() += c;
    }
    return out;
}
}  // namespace codec
}  // namespace hms
