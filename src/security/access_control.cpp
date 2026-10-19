#include "security/access_control.h"

namespace hms {

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
}  // namespace hms
