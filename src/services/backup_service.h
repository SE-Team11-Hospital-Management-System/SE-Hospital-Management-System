// C9 BackupService: time-stamped backups and restore (HMS-F-061).
#pragma once
#include "common/types.h"
#include "security/audit_service.h"
#include "security/session_manager.h"
#include <string>
#include <vector>

namespace hms {
class BackupService {
public:
    BackupService(std::string dataDir, std::string backupRoot, AuditService& audit)
        : dataDir_(std::move(dataDir)), backupRoot_(std::move(backupRoot)), audit_(audit) {}
    Result<std::string> createBackup(const Session& s);                         // returns the folder name YYYYMMDD_HHMMSS
    Result<std::vector<std::string>> listBackups(const Session& s) const;       // newest first
    // Copies the chosen backup over the data folder. The caller must reload all data afterwards.
    Result<void> restore(const Session& s, const std::string& backupName, bool confirm);
    const std::string& backupRoot() const { return backupRoot_; }
    std::string latestBackup() const;                                           // newest folder name, "" if none
private:
    std::string dataDir_, backupRoot_;
    AuditService& audit_;
};
}  // namespace hms
