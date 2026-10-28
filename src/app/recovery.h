// Restore after a failed data check at start-up (HMS-SEC-007). The Administrator logs in against the users file
// inside the latest backup, because the damaged file may be users.dat itself.
#pragma once
#include "app/app_context.h"
#include <string>

namespace hms {
// Returns the name of the restored backup; E001 / E004 if the login is not an Administrator of that backup.
Result<std::string> restoreLatestBackup(AppContext& ctx, const std::string& username, const std::string& password);
}  // namespace hms
