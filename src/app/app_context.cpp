#include "app/app_context.h"
#include "common/datetime.h"

namespace hms {

AppContext::AppContext(const std::string& d, const std::string& backupRoot)
    : dataDir(d),
      audit(d + "/audit.log"),
      sessions(),
      auth(sessions, audit, d),
      patientRepo(),
      patients(patientRepo, audit, d, [] { return dt::today(); }),
      doctors(audit, d, [this](const std::string& userId) {
          auto u = auth.findUser(userId);
          return u && u->role == Role::Doctor && u->active;
      }),
      appointments(patientRepo, doctors, audit, d, [] { return dt::now(); }),
      backup(d, backupRoot, audit) {}
}  // namespace hms
