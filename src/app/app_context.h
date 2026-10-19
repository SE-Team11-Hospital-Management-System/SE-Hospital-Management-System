// Wires every component together once (SAD 3.2 layered architecture). Built in main(), passed to the UI.
#pragma once
#include "data/patient_repository.h"
#include "security/audit_service.h"
#include "security/session_manager.h"
#include "services/appointment_service.h"
#include "services/auth_service.h"
#include "services/backup_service.h"
#include "services/doctor_service.h"
#include "services/patient_service.h"
#include <string>

namespace hms {

struct AppContext {
    std::string dataDir;
    AuditService audit;
    SessionManager sessions;
    AuthService auth;
    PatientRepository patientRepo;
    PatientService patients;
    DoctorService doctors;
    AppointmentService appointments;
    BackupService backup;

    AppContext(const std::string& dataDirectory, const std::string& backupRoot);
};
}  // namespace hms
