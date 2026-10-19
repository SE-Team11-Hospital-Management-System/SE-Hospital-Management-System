// SKELETON STUB (TASK-02). Replaced by the owner's pull request during Sprint 1 - see the header for the owner.
// Owner: Member 3 (Pranav).
#include "services/appointment_service.h"
namespace hms {
const char* toString(AppointmentStatus) { return "?"; }
AppointmentService::AppointmentService(PatientRepository& patients, DoctorService& doctors, AuditService& audit, std::string dataDir, NowFn now)
    : patients_(patients), doctors_(doctors), audit_(audit), path_(dataDir + "/appointments.dat"), now_(std::move(now)) {}
Result<void> AppointmentService::load() { return {}; }
Result<void> AppointmentService::persist() const { return notImplementedError(); }
bool AppointmentService::isSlotFree(const std::string&, const std::string&) const { return false; }
std::vector<std::string> AppointmentService::nextFreeSlots(const std::string&, const std::string&, int) const { return {}; }
Result<std::string> AppointmentService::book(const Session&, const std::string&, const std::string&, const std::string&) { return notImplementedError(); }
Result<std::vector<Appointment>> AppointmentService::forPatient(const Session&, const std::string&) const { return notImplementedError(); }
std::optional<Appointment> AppointmentService::get(const std::string&) const { return std::nullopt; }
std::vector<Appointment> AppointmentService::all() const { return {}; }
}  // namespace hms
