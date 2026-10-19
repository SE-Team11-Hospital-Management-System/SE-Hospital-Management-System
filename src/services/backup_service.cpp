// SKELETON STUB (TASK-02). Replaced by the owner's pull request during Sprint 1 - see the header for the owner.
// Owner: Member 4 (Shristi).
#include "services/backup_service.h"
namespace hms {
Result<std::string> BackupService::createBackup(const Session&) { return notImplementedError(); }
Result<std::vector<std::string>> BackupService::listBackups(const Session&) const { return notImplementedError(); }
Result<void> BackupService::restore(const Session&, const std::string&, bool) { return notImplementedError(); }
}  // namespace hms
