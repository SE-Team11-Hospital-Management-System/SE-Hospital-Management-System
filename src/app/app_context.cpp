#include "app/app_context.h"
#include "common/datetime.h"
#include "security/access_control.h"

namespace hms {

AppContext::AppContext(const std::string& d, const std::string& backupRoot, const std::string& reportsDir)
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
      consultations(appointments, patientRepo, audit, d, [] { return dt::now(); }),
      reports(appointments, doctors, patientRepo, reportsDir),
      backup(d, backupRoot, audit) {
    AccessControl::attach(&sessions, &audit);       // HMS-SEC-002 / F-005: sessions checked, denials audit-logged
}

AppContext::~AppContext() { AccessControl::detach(&sessions); }
}  // namespace hms
