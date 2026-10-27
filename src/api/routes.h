// Every feature registers its own web addresses (routes) in its own file, so owners never edit the same file:
//   routes_status.cpp        /api/status, /api/setup                          (Member 2, foundation)
//   routes_auth.cpp          login, logout, password, users, audit log        (Member 1, SF-1)
//   routes_patients.cpp      patients                                         (Member 2, SF-2)
//   routes_recovery.cpp      restore after a failed data check                (Member 2, SF-2)
//   routes_doctors.cpp       doctors and availability                         (Member 3, SF-3)
//   routes_appointments.cpp  appointments and schedules                       (Member 3, SF-3)
//   routes_clinical.cpp      consultations                                    (Member 4, SF-4)
//   routes_reports.cpp       reports and CSV export                           (Member 4, SF-4)
//   routes_backup.cpp        backup and restore                               (Member 4, SF-4)
#pragma once
#include "api/api.h"

namespace hms {
void registerStatusRoutes(Api& api);
void registerAuthRoutes(Api& api);
void registerPatientRoutes(Api& api);
void registerRecoveryRoutes(Api& api);
void registerDoctorRoutes(Api& api);
void registerAppointmentRoutes(Api& api);
void registerClinicalRoutes(Api& api);
void registerReportRoutes(Api& api);
void registerBackupRoutes(Api& api);

inline void registerAllRoutes(Api& api) {
    registerStatusRoutes(api);
    registerAuthRoutes(api);
    registerPatientRoutes(api);
    registerRecoveryRoutes(api);
    registerDoctorRoutes(api);
    registerAppointmentRoutes(api);
    registerClinicalRoutes(api);
    registerReportRoutes(api);
    registerBackupRoutes(api);
}

// The operations a role may use (SRS Table 6.5-1), sent to the website so it shows only those pages.
json permissionsFor(Role role);
}  // namespace hms
