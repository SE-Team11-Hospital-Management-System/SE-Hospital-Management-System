#include "services/patient_service.h"
#include "security/access_control.h"
#include "security/validator.h"

namespace hms {

PatientService::PatientService(PatientRepository& repo, AuditService& audit, std::string dataDir, TodayFn today)
    : repo_(repo), audit_(audit), path_(dataDir + "/patients.dat"), today_(std::move(today)) {}

Result<void> PatientService::load() { return repo_.load(path_); }

// Returns the first validation error. Messages name the field and the rule (HMS-F-011, HMS-NF-006).
Result<void> PatientService::validate(const PatientInput& in, const std::string& today) {
    if (in.fullName.empty()) return makeError(ErrorCode::E010, "Full name: must not be empty.");
    if (in.fullName.size() > 60) return makeError(ErrorCode::E010, "Full name: at most 60 characters.");
    if (!Validator::isValidPatientName(in.fullName)) return makeError(ErrorCode::E010, "Full name: only letters, spaces, dots or hyphens.");
    if (!Validator::isValidDate(in.dob)) return makeError(ErrorCode::E010, "Date of birth: must be a real date in YYYY-MM-DD format.");
    if (!Validator::isDateNotAfter(in.dob, today)) return makeError(ErrorCode::E010, "Date of birth: must not be in the future.");
    if (!Validator::isValidGender(in.gender)) return makeError(ErrorCode::E010, "Gender: must be M, F or O.");
    if (!Validator::isValidPhone(in.phone)) return makeError(ErrorCode::E010, "Phone: must be exactly 10 digits.");
    if (!Validator::isValidText(in.address, 120)) return makeError(ErrorCode::E010, "Address: 1-120 characters.");
    if (!Validator::isValidBloodGroup(in.bloodGroup)) return makeError(ErrorCode::E010, "Blood group: A+, A-, B+, B-, AB+, AB-, O+ or O-.");
    return {};
}

Result<std::string> PatientService::registerPatient(const Session& s, const PatientInput& in, bool confirmDuplicate) {
    if (auto ok = AccessControl::authorize(s, Operation::RegisterPatient, "registerPatient"); !ok.ok()) return ok.error();
    if (auto v = validate(in, today_()); !v.ok()) return v.error();
    std::string existing;
    if (!confirmDuplicate && repo_.existsDuplicate(in.fullName, in.dob, in.phone, &existing))
        return makeError(ErrorCode::E011, "A matching patient exists (ID " + existing + "). Confirm to continue.", existing);
    Patient p{repo_.nextId(), in.fullName, in.dob, in.gender, in.phone, in.address, in.bloodGroup, true, today_()};
    repo_.put(p);
    if (auto w = repo_.persist(path_); !w.ok()) { repo_.remove(p.patientId); return w.error(); }   // HMS-F-060, NF-011
    audit_.log(s.userId, "PATIENT_CREATE", p.patientId, true);
    return p.patientId;
}

Result<PatientPage> PatientService::search(const Session& s, SearchKey key, const std::string& term, int page) const {
    if (auto ok = AccessControl::authorize(s, Operation::SearchPatient, "searchPatient"); !ok.ok()) return ok.error();
    if (!Validator::isValidText(term, 60)) return makeError(ErrorCode::E010, "Search term: 1-60 characters.");
    if (page < 1) return makeError(ErrorCode::E010, "Page: must be 1 or more.");
    std::vector<Patient> all;
    if (key == SearchKey::Id) { if (auto p = repo_.findById(term)) all.push_back(*p); }
    else if (key == SearchKey::Name) all = repo_.findByNameContains(term);
    else all = repo_.findByPhone(term);
    PatientPage out;
    const size_t start = static_cast<size_t>(page - 1) * kPageSize;
    for (size_t i = start; i < all.size() && i < start + kPageSize; ++i) out.items.push_back(all[i]);
    out.hasNext = all.size() > start + kPageSize;
    return out;
}

Result<void> PatientService::updatePatient(const Session& s, const std::string& id, const PatientInput& c) {
    if (auto ok = AccessControl::authorize(s, Operation::UpdatePatient, "updatePatient"); !ok.ok()) return ok.error();
    auto cur = repo_.findById(id);
    if (!cur) return makeError(ErrorCode::E020, "Patient not found.");
    const Patient before = *cur;
    PatientInput merged{c.fullName.empty() ? cur->fullName : c.fullName, cur->dob, cur->gender,
                        c.phone.empty() ? cur->phone : c.phone, c.address.empty() ? cur->address : c.address,
                        c.bloodGroup.empty() ? cur->bloodGroup : c.bloodGroup};
    if (auto v = validate(merged, today_()); !v.ok()) return v.error();
    std::string changed;                                                // field names only, never values (HMS-F-013)
    auto note = [&](const char* name, const std::string& a, const std::string& b) { if (a != b) changed += (changed.empty() ? "" : ",") + std::string(name); };
    note("name", cur->fullName, merged.fullName); note("phone", cur->phone, merged.phone);
    note("address", cur->address, merged.address); note("bloodGroup", cur->bloodGroup, merged.bloodGroup);
    if (changed.empty()) return {};
    cur->fullName = merged.fullName; cur->phone = merged.phone; cur->address = merged.address; cur->bloodGroup = merged.bloodGroup;
    repo_.put(*cur);
    if (auto w = repo_.persist(path_); !w.ok()) { repo_.put(before); return w.error(); }
    audit_.log(s.userId, "PATIENT_UPDATE", id + ":" + changed, true);
    return {};
}
Result<void> PatientService::deactivatePatient(const Session& s, const std::string& id) {
    if (auto ok = AccessControl::authorize(s, Operation::DeactivatePatient, "deactivatePatient"); !ok.ok()) return ok.error();
    auto cur = repo_.findById(id);
    if (!cur) return makeError(ErrorCode::E020, "Patient not found.");
    if (!cur->active) return makeError(ErrorCode::E032, "Action not allowed: patient is already inactive.");
    Patient changed = *cur;
    changed.active = false;
    repo_.put(changed);
    if (auto w = repo_.persist(path_); !w.ok()) { repo_.put(*cur); return w.error(); }
    audit_.log(s.userId, "PATIENT_DEACTIVATE", id, true);
    return {};
}
}  // namespace hms
