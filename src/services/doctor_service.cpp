#include "services/doctor_service.h"
#include "common/codec.h"
#include "common/datetime.h"
#include "data/file_store.h"
#include "security/access_control.h"
#include "security/validator.h"
#include <algorithm>
#include <cstdio>
#include <cctype>
#include <cstdlib>

namespace hms {

namespace {
std::string encodeWindows(const std::vector<Window>& ws) {
    std::string out;
    for (const auto& w : ws) out += (out.empty() ? "" : ";") + std::to_string(w.weekday) + "@" + w.start + "-" + w.end;
    return out;
}
bool decodeWindows(const std::string& s, std::vector<Window>& out) {
    out.clear();
    size_t pos = 0;
    while (pos < s.size()) {
        size_t end = s.find(';', pos); if (end == std::string::npos) end = s.size();
        const std::string part = s.substr(pos, end - pos);               // "1@09:00-13:00"
        if (part.size() != 13 || part[1] != '@' || part[7] != '-') return false;
        out.push_back(Window{part[0] - '0', part.substr(2, 5), part.substr(8, 5)});
        pos = end + 1;
    }
    return true;
}
}  // namespace

DoctorService::DoctorService(AuditService& audit, std::string dataDir, IsDoctorUserFn isDoctorUser)
    : audit_(audit), path_(dataDir + "/doctors.dat"), isDoctorUser_(std::move(isDoctorUser)) {}

bool DoctorService::isValidSlotTime(const std::string& t) {
    if (t.size() != 5 || t[2] != ':' || !std::isdigit(static_cast<unsigned char>(t[0])) || !std::isdigit(static_cast<unsigned char>(t[1]))) return false;
    const std::string mm = t.substr(3, 2);
    if (mm != "00" && mm != "30") return false;                          // HMS-F-021: 30-minute boundary
    return t >= "08:00" && t <= "20:00";
}

Result<void> DoctorService::load() {
    auto r = FileStore::load(path_);
    if (!r.ok()) return r.error();
    doctors_.clear(); counter_ = 0;
    for (const auto& line : r.value()) {
        auto f = codec::split(line);
        Doctor d;
        if (f.size() != 6 || !decodeWindows(f[5], d.windows)) return makeError(ErrorCode::E040, "Data file damaged. Restore from backup?", path_);
        d.doctorId = f[0]; d.fullName = f[1]; d.specialization = f[2]; d.department = f[3]; d.userId = f[4];
        doctors_[d.doctorId] = d;
        counter_ = std::max(counter_, std::atoi(d.doctorId.c_str() + 1));
    }
    return {};
}

Result<void> DoctorService::persist() const {
    std::vector<std::string> lines;
    for (const auto& [id, d] : doctors_)
        lines.push_back(codec::join({d.doctorId, d.fullName, d.specialization, d.department, d.userId, encodeWindows(d.windows)}));
    return FileStore::atomicWrite(path_, lines);
}

Result<void> DoctorService::validateInput(const DoctorInput& in) const {
    if (!Validator::isValidPatientName(in.fullName)) return makeError(ErrorCode::E010, "Full name: 1-60 letters, spaces, dots or hyphens.");
    if (!Validator::isValidText(in.specialization, 40)) return makeError(ErrorCode::E010, "Specialization: 1-40 characters.");
    if (!Validator::isValidText(in.department, 40)) return makeError(ErrorCode::E010, "Department: 1-40 characters.");
    if (!isDoctorUser_ || !isDoctorUser_(in.userId)) return makeError(ErrorCode::E023, "User not found (must be an existing Doctor account).");
    return {};
}

Result<std::string> DoctorService::addDoctor(const Session& s, const DoctorInput& in) {
    if (!AccessControl::isAllowed(s.role, Operation::ManageDoctors)) { audit_.log(s.userId, "ACCESS_DENIED", "addDoctor", false); return deniedError(); }
    if (auto v = validateInput(in); !v.ok()) return v.error();
    for (const auto& [id, d] : doctors_)
        if (d.userId == in.userId) return makeError(ErrorCode::E011, "This Doctor account is already linked to " + id + ".", id);
    char id[16]; std::snprintf(id, sizeof id, "D%03d", counter_ + 1);
    doctors_[id] = Doctor{id, in.fullName, in.specialization, in.department, in.userId, {}};
    if (auto w = persist(); !w.ok()) { doctors_.erase(id); return w.error(); }
    ++counter_;
    audit_.log(s.userId, "DOCTOR_CREATE", id, true);
    return std::string(id);
}

Result<void> DoctorService::updateDoctor(const Session& s, const std::string& doctorId, const DoctorInput& c) {
    if (!AccessControl::isAllowed(s.role, Operation::ManageDoctors)) { audit_.log(s.userId, "ACCESS_DENIED", "updateDoctor", false); return deniedError(); }
    auto it = doctors_.find(doctorId);
    if (it == doctors_.end()) return makeError(ErrorCode::E021, "Doctor not found.");
    const Doctor before = it->second;
    DoctorInput merged{c.fullName.empty() ? before.fullName : c.fullName, c.specialization.empty() ? before.specialization : c.specialization,
                       c.department.empty() ? before.department : c.department, c.userId.empty() ? before.userId : c.userId};
    if (auto v = validateInput(merged); !v.ok()) return v.error();
    it->second.fullName = merged.fullName; it->second.specialization = merged.specialization;
    it->second.department = merged.department; it->second.userId = merged.userId;
    if (auto w = persist(); !w.ok()) { it->second = before; return w.error(); }
    audit_.log(s.userId, "DOCTOR_UPDATE", doctorId, true);
    return {};
}

Result<void> DoctorService::setAvailability(const Session& s, const std::string& doctorId, const std::vector<Window>& ws) {
    if (!AccessControl::isAllowed(s.role, Operation::ManageDoctors)) { audit_.log(s.userId, "ACCESS_DENIED", "setAvailability", false); return deniedError(); }
    auto it = doctors_.find(doctorId);
    if (it == doctors_.end()) return makeError(ErrorCode::E021, "Doctor not found.");
    for (const auto& x : ws) {
        if (x.weekday < 0 || x.weekday > 6) return makeError(ErrorCode::E010, "Weekday: 0 (Sunday) to 6 (Saturday).");
        if (!isValidSlotTime(x.start) || !isValidSlotTime(x.end) || x.start >= x.end)
            return makeError(ErrorCode::E010, "Working hours: start and end on a 30-minute boundary between 08:00 and 20:00, start before end.");
    }
    const auto before = it->second.windows;
    it->second.windows = ws;
    if (auto w = persist(); !w.ok()) { it->second.windows = before; return w.error(); }
    audit_.log(s.userId, "DOCTOR_AVAILABILITY", doctorId, true);
    return {};
}

Result<std::vector<Doctor>> DoctorService::list(const Session& s, const std::optional<std::string>& spec) const {
    if (!AccessControl::isAllowed(s.role, Operation::ListDoctors)) { audit_.log(s.userId, "ACCESS_DENIED", "listDoctors", false); return deniedError(); }
    std::vector<Doctor> out;
    for (const auto& [id, d] : doctors_) if (!spec || d.specialization == *spec) out.push_back(d);
    return out;
}

std::optional<Doctor> DoctorService::get(const std::string& doctorId) const {
    auto it = doctors_.find(doctorId);
    if (it == doctors_.end()) return std::nullopt;
    return it->second;
}

bool DoctorService::isWithinAvailability(const std::string& doctorId, const std::string& slotStart) const {
    auto it = doctors_.find(doctorId);
    if (it == doctors_.end()) return false;
    const int wd = dt::weekday(slotStart);
    const std::string t = dt::timeOf(slotStart), tEnd = dt::timeOf(dt::addMinutes(slotStart, 30));
    for (const auto& w : it->second.windows)
        if (w.weekday == wd && t >= w.start && tEnd <= w.end && tEnd > t) return true;
    return false;
}
}  // namespace hms
