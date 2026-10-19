#include "data/patient_repository.h"
#include "common/codec.h"
#include "data/file_store.h"
#include <algorithm>
#include <cctype>
#include <cstdio>

namespace hms {

namespace {
std::string lower(std::string s) { std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); }); return s; }
}

std::string PatientRepository::nextId() const { char b[16]; std::snprintf(b, sizeof b, "P%06d", counter_ + 1); return b; }

void PatientRepository::put(const Patient& p) {
    byId_[p.patientId] = p;
    if (p.patientId.size() == 7) counter_ = std::max(counter_, std::atoi(p.patientId.c_str() + 1));
}

std::optional<Patient> PatientRepository::findById(const std::string& id) const {
    auto it = byId_.find(id);
    if (it == byId_.end()) return std::nullopt;
    return it->second;
}

std::vector<Patient> PatientRepository::findByNameContains(const std::string& fragment) const {
    std::vector<Patient> out; const std::string f = lower(fragment);
    for (const auto& [id, p] : byId_) if (lower(p.fullName).find(f) != std::string::npos) out.push_back(p);
    return out;
}

std::vector<Patient> PatientRepository::findByPhone(const std::string& phone) const {
    std::vector<Patient> out;
    for (const auto& [id, p] : byId_) if (p.phone == phone) out.push_back(p);
    return out;
}

bool PatientRepository::existsDuplicate(const std::string& name, const std::string& dob, const std::string& phone, std::string* existingId) const {
    for (const auto& [id, p] : byId_)
        if (lower(p.fullName) == lower(name) && p.dob == dob && p.phone == phone) { if (existingId) *existingId = id; return true; }
    return false;
}

Result<void> PatientRepository::load(const std::string& path) {
    auto r = FileStore::load(path);
    if (!r.ok()) return r.error();
    byId_.clear(); counter_ = 0;
    for (const auto& line : r.value()) {
        auto f = codec::split(line);
        if (f.size() != 9) return makeError(ErrorCode::E040, "Data file damaged. Restore from backup?", path);
        put(Patient{f[0], f[1], f[2], f[3], f[4], f[5], f[6], f[7] == "1", f[8]});
    }
    return {};
}

Result<void> PatientRepository::persist(const std::string& path) const {
    std::vector<std::string> lines;
    lines.reserve(byId_.size());
    for (const auto& [id, p] : byId_)
        lines.push_back(codec::join({p.patientId, p.fullName, p.dob, p.gender, p.phone, p.address, p.bloodGroup, p.active ? "1" : "0", p.registeredOn}));
    return FileStore::atomicWrite(path, lines);
}
}  // namespace hms
