// C10 AccessControl: deny-by-default RBAC (HMS-F-003, HMS-SEC-002). Matrix = SRS Table 6.5-1.
#pragma once
#include "common/types.h"
namespace hms {

enum class Operation {
    Login, ChangeOwnPassword, ManageUsers, ViewAuditLog,
    RegisterPatient, UpdatePatient, SearchPatient, DeactivatePatient,
    ManageDoctors, ListDoctors,
    BookAppointment,          // also cancel / reschedule / mark no-show
    ViewSchedule,
    RecordConsultation, ViewMedicalHistory,
    DailyReport, WorkloadReport, Backup
};

class AccessControl {
public:
    static bool isAllowed(Role role, Operation op);
};
}  // namespace hms
