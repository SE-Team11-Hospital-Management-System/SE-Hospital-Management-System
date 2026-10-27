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
std::string key(const std::string& doctorId, const std::string& slot) { return doctorId + "|" + slot; }
Error notAllowed(AppointmentStatus s) { return makeError(ErrorCode::E032, std::string("Action not allowed for status ") + toString(s) + "."); }
}  // namespace

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
        if (blocksSlot(a.status)) taken_[key(a.doctorId, a.slotStart)] = a.appointmentId;
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
    return taken_.find(key(doctorId, slot)) == taken_.end();
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

// Why a slot cannot be booked for this doctor, with the next 3 free slots in details (HMS-F-031).
std::optional<Error> AppointmentService::slotProblem(const std::string& doctorId, const std::string& slot) const {
    if (!doctors_.exists(doctorId)) return makeError(ErrorCode::E021, "Doctor not found.");
    if (!dt::isValid(slot) || !dt::onHalfHour(slot)) return makeError(ErrorCode::E010, "Slot: YYYY-MM-DD HH:MM on a 30-minute boundary.");
    std::string alt;
    for (const auto& x : nextFreeSlots(doctorId, std::max(slot, now_()), 3)) alt += (alt.empty() ? "" : ", ") + x;
    if (slot <= now_()) return makeError(ErrorCode::E031, "Slot is in the past. Next free slots: " + alt, alt);
    if (!doctors_.isWithinAvailability(doctorId, slot)) return makeError(ErrorCode::E031, "Slot is outside the doctor's availability. Next free slots: " + alt, alt);
    if (!isSlotFree(doctorId, slot)) return makeError(ErrorCode::E030, "Slot taken. Next free slots: " + alt, alt);
    return std::nullopt;
}

Result<std::string> AppointmentService::book(const Session& s, const std::string& patientId, const std::string& doctorId, const std::string& slotStart) {
    if (auto ok = AccessControl::authorize(s, Operation::BookAppointment, "book"); !ok.ok()) return ok.error();
    auto p = patients_.findById(patientId);
    if (!p) return makeError(ErrorCode::E020, "Patient not found.");
    if (!p->active) return makeError(ErrorCode::E025, "Patient record is inactive.");
    if (auto problem = slotProblem(doctorId, slotStart)) return *problem;
    char id[16]; std::snprintf(id, sizeof id, "A%08d", counter_ + 1);
    appts_[id] = Appointment{id, patientId, doctorId, slotStart, s.userId, AppointmentStatus::Scheduled};
    taken_[key(doctorId, slotStart)] = id;
    if (auto w = persist(); !w.ok()) { appts_.erase(id); taken_.erase(key(doctorId, slotStart)); return w.error(); }
    ++counter_;
    audit_.log(s.userId, "APPT_BOOKED", id, true);
    return std::string(id);
}

bool AppointmentService::canChange(AppointmentStatus from, AppointmentStatus to) {
    return from == AppointmentStatus::Scheduled && to != AppointmentStatus::Scheduled;
}

// Applies an allowed status change in memory and on disk; undoes it if the save fails.
Result<void> AppointmentService::setStatus(Appointment& a, AppointmentStatus to) {
    const AppointmentStatus before = a.status;
    a.status = to;
    if (!blocksSlot(to)) taken_.erase(key(a.doctorId, a.slotStart));   // a cancelled slot is free again at once (HMS-F-032)
    if (auto w = persist(); !w.ok()) {
        a.status = before;
        if (blocksSlot(before)) taken_[key(a.doctorId, a.slotStart)] = a.appointmentId;
        return w.error();
    }
    return {};
}

Result<void> AppointmentService::changeStatus(const Session& s, const std::string& id, AppointmentStatus to) {
    if (auto ok = AccessControl::authorize(s, Operation::BookAppointment, "changeStatus"); !ok.ok()) return ok.error();
    auto it = appts_.find(id);
    if (it == appts_.end()) return makeError(ErrorCode::E022, "Appointment not found.");
    Appointment& a = it->second;
    if (!canChange(a.status, to) || to == AppointmentStatus::Completed) return notAllowed(a.status);
    const bool started = a.slotStart <= now_();
    if (to == AppointmentStatus::Cancelled && started)
        return makeError(ErrorCode::E032, "Action not allowed: the appointment has already started.");
    if (to == AppointmentStatus::NoShow && !started)
        return makeError(ErrorCode::E032, "Action not allowed: No-Show only after the start time.");
    if (auto w = setStatus(a, to); !w.ok()) return w.error();
    audit_.log(s.userId, to == AppointmentStatus::Cancelled ? "APPT_CANCELLED" : "APPT_NO_SHOW", id, true);
    return {};
}

Result<void> AppointmentService::markCompleted(const std::string& id) {
    auto it = appts_.find(id);
    if (it == appts_.end()) return makeError(ErrorCode::E022, "Appointment not found.");
    if (!canChange(it->second.status, AppointmentStatus::Completed)) return notAllowed(it->second.status);
    return setStatus(it->second, AppointmentStatus::Completed);
}

Result<std::vector<Appointment>> AppointmentService::forPatient(const Session& s, const std::string& patientId) const {
    if (auto ok = AccessControl::authorize(s, Operation::BookAppointment, "listAppointments"); !ok.ok()) return ok.error();
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
