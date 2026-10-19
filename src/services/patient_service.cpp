// SKELETON STUB (TASK-02). Replaced by the owner's pull request during Sprint 1 - see the header for the owner.
// Owner: Member 2 (Saurav).
#include "services/patient_service.h"
namespace hms {
PatientService::PatientService(PatientRepository& repo, AuditService& audit, std::string dataDir, TodayFn today)
    : repo_(repo), audit_(audit), path_(dataDir + "/patients.dat"), today_(std::move(today)) {}
Result<void> PatientService::load() { return {}; }
Result<void> PatientService::validate(const PatientInput&, const std::string&) { return notImplementedError(); }
Result<std::string> PatientService::registerPatient(const Session&, const PatientInput&, bool) { return notImplementedError(); }
Result<PatientPage> PatientService::search(const Session&, SearchKey, const std::string&, int) const { return notImplementedError(); }
Result<void> PatientService::updatePatient(const Session&, const std::string&, const PatientInput&) { return notImplementedError(); }
}  // namespace hms
