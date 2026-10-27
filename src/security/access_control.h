// C10 AccessControl: deny-by-default RBAC (HMS-F-003, HMS-SEC-002). Matrix = SRS Table 6.5-1.
// Every service operation starts with AccessControl::authorize(), which checks the role first (E004, audit-logged)
// and then that the session is still valid (E003 after 10 minutes without activity, HMS-F-005).
#pragma once
#include "common/types.h"
#include "security/session_manager.h"

namespace hms {

class AuditService;

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
    // HMS-SEC-002: call first in every service operation. 'action' is written to the audit log when denied.
    static Result<void> authorize(const Session& s, Operation op, const std::string& action);
    // Done once by AppContext: from then on sessions are checked and denials are audit-logged.
    static void attach(SessionManager* sessions, AuditService* audit);
    static void detach(const SessionManager* sessions);   // only if 'sessions' is the attached one
};
}  // namespace hms
