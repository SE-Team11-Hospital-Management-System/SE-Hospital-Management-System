// SKELETON STUB (TASK-02). Replaced by the owner's pull request during Sprint 1 - see the header for the owner.
// Owner: Member 3 (Pranav).
#include "services/doctor_service.h"
namespace hms {
DoctorService::DoctorService(AuditService& audit, std::string dataDir, IsDoctorUserFn isDoctorUser)
    : audit_(audit), path_(dataDir + "/doctors.dat"), isDoctorUser_(std::move(isDoctorUser)) {}
bool DoctorService::isValidSlotTime(const std::string&) { return false; }
Result<void> DoctorService::load() { return {}; }
Result<void> DoctorService::persist() const { return notImplementedError(); }
Result<void> DoctorService::validateInput(const DoctorInput&) const { return notImplementedError(); }
Result<std::string> DoctorService::addDoctor(const Session&, const DoctorInput&) { return notImplementedError(); }
Result<void> DoctorService::updateDoctor(const Session&, const std::string&, const DoctorInput&) { return notImplementedError(); }
Result<void> DoctorService::setAvailability(const Session&, const std::string&, const std::vector<Window>&) { return notImplementedError(); }
Result<std::vector<Doctor>> DoctorService::list(const Session&, const std::optional<std::string>&) const { return notImplementedError(); }
std::optional<Doctor> DoctorService::get(const std::string&) const { return std::nullopt; }
bool DoctorService::isWithinAvailability(const std::string&, const std::string&) const { return false; }
}  // namespace hms
