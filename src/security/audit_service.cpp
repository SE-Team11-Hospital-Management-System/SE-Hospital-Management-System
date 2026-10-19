#include "security/audit_service.h"
#include "common/codec.h"
#include "common/datetime.h"
#include <filesystem>
#include <fstream>

namespace hms {

AuditService::AuditService(std::string logPath) : path_(std::move(logPath)) {}

void AuditService::log(const std::string& userId, const std::string& action, const std::string& targetId, bool success) {
    const bool isNew = !std::filesystem::exists(path_);
    std::ofstream out(path_, std::ios::app);                      // append only
    if (!out) return;                                             // auditing must never crash the program
    out << codec::join({dt::isoNow(), userId.empty() ? "-" : userId, action, targetId.empty() ? "-" : targetId,
                        success ? "SUCCESS" : "FAILURE"}) << '\n';
    out.close();
    if (isNew) {                                                  // owner-only (HMS-SEC-006)
        std::error_code ec;
        std::filesystem::permissions(path_, std::filesystem::perms::owner_read | std::filesystem::perms::owner_write,
                                     std::filesystem::perm_options::replace, ec);
    }
}

std::vector<AuditEntry> AuditService::readAll() const {
    std::vector<AuditEntry> v;
    std::ifstream in(path_);
    std::string line;
    while (std::getline(in, line)) {
        auto f = codec::split(line);
        if (f.size() != 5) continue;
        v.push_back(AuditEntry{f[0], f[1], f[2], f[3], f[4]});
    }
    return v;
}
}  // namespace hms
