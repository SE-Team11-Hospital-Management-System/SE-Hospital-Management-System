// C12 AuditService: append-only audit log (HMS-SEC-003). Never logs passwords or clinical text.
#pragma once
#include <string>
#include <vector>
namespace hms {

struct AuditEntry {
    std::string timestamp;   // ISO 8601
    std::string userId;      // "-" if not logged in
    std::string action;      // LOGIN_OK, LOGIN_FAIL, ACCOUNT_LOCKED, ACCESS_DENIED, ...
    std::string targetId;
    std::string outcome;     // SUCCESS / FAILURE
};

class AuditService {
public:
    explicit AuditService(std::string logPath);
    void log(const std::string& userId, const std::string& action, const std::string& targetId, bool success);
    std::vector<AuditEntry> readAll() const;
    const std::string& path() const { return path_; }
    // Deliberately no edit / delete API (HMS-SEC-003).
private:
    std::string path_;
};
}  // namespace hms
