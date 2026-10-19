// C6 AppointmentService: booking with slot-conflict detection (HMS-F-030/031).
// Cancel / reschedule / schedule view / status life-cycle arrive in Sprint 2.
// Saved to <dataDir>/appointments.dat:  appointmentId|patientId|doctorId|slotStart|bookedBy|status(0..3)
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
private:
    bool isSlotFree(const std::string& doctorId, const std::string& slot) const;
    Result<void> persist() const;
    [[maybe_unused]] PatientRepository& patients_;
    [[maybe_unused]] DoctorService& doctors_;
    [[maybe_unused]] AuditService& audit_;
    std::string path_;
    NowFn now_;
    std::map<std::string, Appointment> appts_;
    std::map<std::string, std::string> taken_;                              // "doctorId|slot" -> appointmentId (fast conflict check)
    [[maybe_unused]] int counter_{0};
};
}  // namespace hms
