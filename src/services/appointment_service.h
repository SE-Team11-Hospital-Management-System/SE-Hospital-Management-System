// C6 AppointmentService: booking with slot-conflict detection, cancel, no-show and the status life-cycle
// (HMS-F-030, F-031, F-032, F-035). Reschedule and daily schedules follow. Saved to <dataDir>/appointments.dat:
//   appointmentId|patientId|doctorId|slotStart|bookedBy|status(0=Scheduled,1=Completed,2=Cancelled,3=NoShow)
#pragma once
#include "common/types.h"
#include "data/patient_repository.h"
#include "security/audit_service.h"
#include "security/session_manager.h"
#include "services/doctor_service.h"
#include <functional>
#include <map>
#include <optional>
#include <vector>

namespace hms {

enum class AppointmentStatus { Scheduled, Completed, Cancelled, NoShow };
const char* toString(AppointmentStatus s);

struct Appointment {
    std::string appointmentId, patientId, doctorId, slotStart, bookedBy;   // slotStart = "YYYY-MM-DD HH:MM"
    AppointmentStatus status{AppointmentStatus::Scheduled};
};

class AppointmentService {
public:
    using NowFn = std::function<std::string()>;                            // returns "YYYY-MM-DD HH:MM"
    AppointmentService(PatientRepository& patients, DoctorService& doctors, AuditService& audit, std::string dataDir, NowFn now);

    Result<void> load();
    // E020/E025 patient, E021 doctor, E031 past / outside availability, E030 taken (details = next 3 free slots, comma separated)
    Result<std::string> book(const Session& s, const std::string& patientId, const std::string& doctorId, const std::string& slotStart);
    std::vector<std::string> nextFreeSlots(const std::string& doctorId, const std::string& from, int n) const;
    Result<std::vector<Appointment>> forPatient(const Session& s, const std::string& patientId) const;   // ascending time
    std::optional<Appointment> get(const std::string& id) const;
    std::vector<Appointment> all() const;

    // HMS-F-035: Scheduled -> Completed | Cancelled | NoShow are the only allowed changes (E032 otherwise).
    static bool canChange(AppointmentStatus from, AppointmentStatus to);
    // Cancelled (before the start time) or NoShow (after the start time). Completed is set only by markCompleted().
    Result<void> changeStatus(const Session& s, const std::string& id, AppointmentStatus to);
    Result<void> cancel(const Session& s, const std::string& id) { return changeStatus(s, id, AppointmentStatus::Cancelled); }
    Result<void> markNoShow(const Session& s, const std::string& id) { return changeStatus(s, id, AppointmentStatus::NoShow); }
    Result<void> markCompleted(const std::string& id);                     // called by ConsultationService after saving


private:
    std::optional<Error> slotProblem(const std::string& doctorId, const std::string& slot) const;
    Result<void> setStatus(Appointment& a, AppointmentStatus to);
    bool isSlotFree(const std::string& doctorId, const std::string& slot) const;
    Result<void> persist() const;
    PatientRepository& patients_;
    DoctorService& doctors_;
    AuditService& audit_;
    std::string path_;
    NowFn now_;
    std::map<std::string, Appointment> appts_;
    std::map<std::string, std::string> taken_;                              // "doctorId|slot" -> appointmentId (fast conflict check)
    int counter_{0};
};
}  // namespace hms
