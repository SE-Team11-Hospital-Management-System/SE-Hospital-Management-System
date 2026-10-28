// C4 PatientService: register, search, update, deactivate (HMS-F-010..015, SEC-005).
#pragma once
#include "common/types.h"
#include "data/patient_repository.h"
#include "security/audit_service.h"
#include "security/session_manager.h"
#include <functional>
#include <vector>

namespace hms {

struct PatientInput { std::string fullName, dob, gender, phone, address, bloodGroup; };
enum class SearchKey { Id, Name, Phone };
struct PatientPage { std::vector<Patient> items; bool hasNext{false}; };

class PatientService {
public:
    using TodayFn = std::function<std::string()>;                      // returns "YYYY-MM-DD"
    PatientService(PatientRepository& repo, AuditService& audit, std::string dataDir, TodayFn today);

    Result<void> load();                                               // reads patients.dat
    // E011 (with the existing ID in details) when a duplicate exists and confirmDuplicate is false (HMS-F-015).
    Result<std::string> registerPatient(const Session& s, const PatientInput& in, bool confirmDuplicate = false);
    Result<PatientPage> search(const Session& s, SearchKey key, const std::string& term, int page = 1) const;
    // Empty fields in 'changes' mean "keep". Patient ID and date of birth cannot change (HMS-F-013).
    Result<void> updatePatient(const Session& s, const std::string& id, const PatientInput& changes);
    // HMS-F-014: Administrator only. The record is kept (never deleted) and marked Inactive.
    Result<void> deactivatePatient(const Session& s, const std::string& id);
    std::optional<Patient> get(const std::string& id) const { return repo_.findById(id); }

    static constexpr size_t kPageSize = 20;
    static Result<void> validate(const PatientInput& in, const std::string& today);   // field-specific E010
private:
    PatientRepository& repo_;
    AuditService& audit_;
    std::string path_;
    TodayFn today_;
};
}  // namespace hms
