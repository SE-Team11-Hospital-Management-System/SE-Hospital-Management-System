// SKELETON STUB (TASK-02). Replaced by the owner's pull request during Sprint 1 - see the header for the owner.
// Owner: Member 1 (Paras).
#include "security/audit_service.h"
namespace hms {
AuditService::AuditService(std::string logPath) : path_(std::move(logPath)) {}
void AuditService::log(const std::string&, const std::string&, const std::string&, bool) {}
std::vector<AuditEntry> AuditService::readAll() const { return {}; }
}  // namespace hms
