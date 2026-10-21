#include "services/appointment_service.h"
#include "common/codec.h"
#include "common/datetime.h"
#include "data/file_store.h"
#include "security/access_control.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>

namespace hms {

const char* toString(AppointmentStatus s) {
    switch (s) {
        case AppointmentStatus::Scheduled: return "SCHEDULED";
        case AppointmentStatus::Completed: return "COMPLETED";
        case AppointmentStatus::Cancelled: return "CANCELLED";
        case AppointmentStatus::NoShow:    return "NO_SHOW";
    }
    return "?";
}

namespace {
bool blocksSlot(AppointmentStatus s) { return s == AppointmentStatus::Scheduled || s == AppointmentStatus::Completed; }   // HMS-F-031
}

AppointmentService::AppointmentService(PatientRepository& patients, DoctorService& doctors, AuditService& audit, std::string dataDir, NowFn now)
    : patients_(patients), doctors_(doctors), audit_(audit), path_(dataDir + "/appointments.dat"), now_(std::move(now)) {}

Result<void> AppointmentService::load() {
    auto r = FileStore::load(path_);
    if (!r.ok()) return r.error();
    appts_.clear(); taken_.clear(); counter_ = 0;
    for (const auto& line : r.value()) {
        auto f = codec::split(line);
        const int st = f.size() == 6 ? std::atoi(f[5].c_str()) : -1;
        if (st < 0 || st > 3) return makeError(ErrorCode::E040, "Data file damaged. Restore from backup?", path_);
        Appointment a{f[0], f[1], f[2], f[3], f[4], static_cast<AppointmentStatus>(st)};
        appts_[a.appointmentId] = a;
        if (blocksSlot(a.status)) taken_[a.doctorId + "|" + a.slotStart] = a.appointmentId;
        counter_ = std::max(counter_, std::atoi(a.appointmentId.c_str() + 1));
    }
    return {};
}

Result<void> AppointmentService::persist() const {
    std::vector<std::string> lines;
    lines.reserve(appts_.size());
    for (const auto& [id, a] : appts_)
        lines.push_back(codec::join({a.appointmentId, a.patientId, a.doctorId, a.slotStart, a.bookedBy, std::to_string(static_cast<int>(a.status))}));
    return FileStore::atomicWrite(path_, lines);
}

bool AppointmentService::isSlotFree(const std::string& doctorId, const std::string& slot) const {
    return taken_.find(doctorId + "|" + slot) == taken_.end();
}

std::vector<std::string> AppointmentService::nextFreeSlots(const std::string& doctorId, const std::string& from, int n) const {
    std::vector<std::string> out;
    std::string s = from;
    if (!dt::onHalfHour(s)) s = s.substr(0, 14) + (s.substr(14, 2) < "30" ? "00" : "30");
    const std::string now = now_();
    for (int i = 0; i < 14 * 48 && static_cast<int>(out.size()) < n; ++i) {   // look ahead 14 days
        s = dt::addMinutes(s, 30);
        if (s > now && doctors_.isWithinAvailability(doctorId, s) && isSlotFree(doctorId, s)) out.push_back(s);
    }
    return out;
}

Result<std::string> AppointmentService::book(const Session& s, const std::string& patientId, const std::string& doctorId, const std::string& slotStart) {
    if (!AccessControl::isAllowed(s.role, Operation::BookAppointment)) { audit_.log(s.userId, "ACCESS_DENIED", "book", false); return deniedError(); }
    auto p = patients_.findById(patientId);
    if (!p) return makeError(ErrorCode::E020, "Patient not found.");
    if (!p->active) return makeError(ErrorCode::E025, "Patient record is inactive.");
    if (!doctors_.exists(doctorId)) return makeError(ErrorCode::E021, "Doctor not found.");
    if (!dt::isValid(slotStart) || !dt::onHalfHour(slotStart)) return makeError(ErrorCode::E010, "Slot: YYYY-MM-DD HH:MM on a 30-minute boundary.");
    auto alternatives = [&] {
        std::string alt;
        for (const auto& x : nextFreeSlots(doctorId, std::max(slotStart, now_()), 3)) alt += (alt.empty() ? "" : ", ") + x;
        return alt;
    };
    if (slotStart <= now_()) { auto a = alternatives(); return makeError(ErrorCode::E031, "Slot is in the past. Next free slots: " + a, a); }
    if (!doctors_.isWithinAvailability(doctorId, slotStart)) { auto a = alternatives(); return makeError(ErrorCode::E031, "Slot is outside the doctor's availability. Next free slots: " + a, a); }
    if (!isSlotFree(doctorId, slotStart)) { auto a = alternatives(); return makeError(ErrorCode::E030, "Slot taken. Next free slots: " + a, a); }
    char id[16]; std::snprintf(id, sizeof id, "A%08d", counter_ + 1);
    appts_[id] = Appointment{id, patientId, doctorId, slotStart, s.userId, AppointmentStatus::Scheduled};
    taken_[doctorId + "|" + slotStart] = id;
    if (auto w = persist(); !w.ok()) { appts_.erase(id); taken_.erase(doctorId + "|" + slotStart); return w.error(); }
    ++counter_;
    audit_.log(s.userId, "APPT_BOOKED", id, true);
    return std::string(id);
}

Result<std::vector<Appointment>> AppointmentService::forPatient(const Session& s, const std::string& patientId) const {
    if (!AccessControl::isAllowed(s.role, Operation::BookAppointment)) return deniedError();
    if (!patients_.findById(patientId)) return makeError(ErrorCode::E020, "Patient not found.");
    std::vector<Appointment> out;
    for (const auto& [id, a] : appts_) if (a.patientId == patientId) out.push_back(a);
    std::sort(out.begin(), out.end(), [](const Appointment& x, const Appointment& y) { return x.slotStart < y.slotStart; });
    return out;
}

std::optional<Appointment> AppointmentService::get(const std::string& id) const {
    auto it = appts_.find(id);
    if (it == appts_.end()) return std::nullopt;
    return it->second;
}

std::vector<Appointment> AppointmentService::all() const {
    std::vector<Appointment> v;
    v.reserve(appts_.size());
    for (const auto& [id, a] : appts_) v.push_back(a);
    return v;
}
}  // namespace hms
