#include "security/access_control.h"
#include "security/audit_service.h"

namespace hms {

namespace {
SessionManager* g_sessions = nullptr;
AuditService* g_audit = nullptr;
}  // namespace

bool AccessControl::isAllowed(Role role, Operation op) {
    const bool A = role == Role::Admin, R = role == Role::Receptionist, D = role == Role::Doctor;
    switch (op) {
        case Operation::Login:
        case Operation::ChangeOwnPassword:   return true;
        case Operation::ManageUsers:
        case Operation::ViewAuditLog:
        case Operation::DeactivatePatient:
        case Operation::ManageDoctors:
        case Operation::WorkloadReport:
        case Operation::Backup:              return A;
        case Operation::RegisterPatient:
        case Operation::UpdatePatient:
        case Operation::BookAppointment:
        case Operation::DailyReport:         return A || R;
        case Operation::SearchPatient:
        case Operation::ListDoctors:
        case Operation::ViewSchedule:        return A || R || D;   // Doctor: own schedule only (checked in service)
        case Operation::RecordConsultation:
        case Operation::ViewMedicalHistory:  return D;             // HMS-F-042: clinical data = Doctors only
    }
    return false;                                                  // deny by default
}

Result<void> AccessControl::authorize(const Session& s, Operation op, const std::string& action) {
    if (!isAllowed(s.role, op)) {
        if (g_audit) g_audit->log(s.userId, "ACCESS_DENIED", action, false);
        return deniedError();
    }
    if (g_sessions) {
        auto v = g_sessions->validate(s.token);                    // also refreshes the idle timer
        if (!v.ok()) return v.error();
    }
    return {};
}

void AccessControl::attach(SessionManager* sessions, AuditService* audit) { g_sessions = sessions; g_audit = audit; }

void AccessControl::detach(const SessionManager* sessions) {
    if (g_sessions == sessions) { g_sessions = nullptr; g_audit = nullptr; }
}
}  // namespace hms
