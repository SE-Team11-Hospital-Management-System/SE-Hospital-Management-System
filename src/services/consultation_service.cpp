#include "services/consultation_service.h"
#include "common/codec.h"
#include "common/datetime.h"
#include "data/file_store.h"
#include "security/access_control.h"
#include "security/validator.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>

namespace hms {

namespace {
Error damaged(const std::string& path) { return makeError(ErrorCode::E040, "Data file damaged. Restore from backup?", path); }
bool okText(const std::string& s, size_t maxLen) { return s.size() <= maxLen && (s.empty() || Validator::isValidText(s, maxLen)); }
}  // namespace

ConsultationService::ConsultationService(AppointmentService& appts, PatientRepository& patients, AuditService& audit,
                                         std::string dataDir, NowFn now)
    : appts_(appts), patients_(patients), audit_(audit), dir_(std::move(dataDir)), now_(std::move(now)) {}

int ConsultationService::ageOn(const std::string& dob, const std::string& today) {
    if (dob.size() < 10 || today.size() < 10) return 0;
    int age = std::atoi(today.substr(0, 4).c_str()) - std::atoi(dob.substr(0, 4).c_str());
    if (today.substr(5, 5) < dob.substr(5, 5)) --age;                  // birthday not reached yet this year
    return std::max(age, 0);
}

Result<void> ConsultationService::validate(const ConsultationInput& in) {
    if (in.diagnosis.empty()) return makeError(ErrorCode::E010, "Diagnosis: must not be empty.");
    if (!okText(in.diagnosis, kMaxDiagnosis)) return makeError(ErrorCode::E010, "Diagnosis: at most 500 characters.");
    if (in.items.size() > kMaxItems) return makeError(ErrorCode::E010, "Prescription: at most 10 items.");
    for (const auto& it : in.items) {
        if (!Validator::isValidText(it.medicine, 60)) return makeError(ErrorCode::E010, "Medicine: 1-60 characters.");
        if (!Validator::isValidText(it.dosage, 40)) return makeError(ErrorCode::E010, "Dosage: 1-40 characters.");
        if (it.days < 1 || it.days > 365) return makeError(ErrorCode::E010, "Duration: 1-365 days.");
    }
    if (!okText(in.notes, kMaxNotes)) return makeError(ErrorCode::E010, "Notes: at most 1000 characters.");
    return {};
}

Result<void> ConsultationService::load() {
    auto c = FileStore::load(dir_ + "/consultations.dat");
    auto p = FileStore::load(dir_ + "/prescriptions.dat");
    auto a = FileStore::load(dir_ + "/addenda.dat");
    if (!c.ok()) return c.error();
    if (!p.ok()) return p.error();
    if (!a.ok()) return a.error();
    byId_.clear(); counter_ = 0;
    for (const auto& line : c.value()) {
        auto f = codec::split(line);
        if (f.size() != 7) return damaged(dir_ + "/consultations.dat");
        byId_[f[0]] = Consultation{f[0], f[1], f[2], f[3], f[4], f[5], f[6], {}, {}};
        counter_ = std::max(counter_, std::atoi(f[0].c_str() + 1));
    }
    for (const auto& line : p.value()) {
        auto f = codec::split(line);
        if (f.size() != 5 || !byId_.count(f[0])) return damaged(dir_ + "/prescriptions.dat");
        byId_[f[0]].items.push_back(PrescriptionItem{f[2], f[3], std::atoi(f[4].c_str())});
    }
    for (const auto& line : a.value()) {
        auto f = codec::split(line);
        if (f.size() != 4 || !byId_.count(f[0])) return damaged(dir_ + "/addenda.dat");
        byId_[f[0]].addenda.push_back(Addendum{f[1], f[2], f[3]});
    }
    return {};
}

Result<void> ConsultationService::persist() const {
    std::vector<std::string> cons, items, adds;
    for (const auto& [id, c] : byId_) {
        cons.push_back(codec::join({c.consultId, c.appointmentId, c.patientId, c.doctorId, c.createdAt, c.diagnosis, c.notes}));
        for (size_t i = 0; i < c.items.size(); ++i)
            items.push_back(codec::join({id, std::to_string(i + 1), c.items[i].medicine, c.items[i].dosage, std::to_string(c.items[i].days)}));
        for (const auto& a : c.addenda) adds.push_back(codec::join({id, a.timestamp, a.authorUserId, a.text}));
    }
    if (auto w = FileStore::atomicWrite(dir_ + "/prescriptions.dat", items); !w.ok()) return w;
    if (auto w = FileStore::atomicWrite(dir_ + "/addenda.dat", adds); !w.ok()) return w;
    return FileStore::atomicWrite(dir_ + "/consultations.dat", cons);
}

// The appointment must exist, belong to the logged-in Doctor and still be Scheduled.
Result<Appointment> ConsultationService::ownScheduledAppointment(const Session& s, const std::string& appointmentId) const {
    if (auto ok = AccessControl::authorize(s, Operation::RecordConsultation, "recordConsultation"); !ok.ok()) return ok.error();
    auto a = appts_.get(appointmentId);
    if (!a) return makeError(ErrorCode::E022, "Appointment not found.");
    if (s.doctorId.empty() || a->doctorId != s.doctorId) {
        audit_.log(s.userId, "ACCESS_DENIED", "recordConsultation:" + appointmentId, false);
        return deniedError();
    }
    if (a->status != AppointmentStatus::Scheduled)
        return makeError(ErrorCode::E032, std::string("Action not allowed for status ") + toString(a->status) + ".");
    return *a;
}

Result<PatientSummary> ConsultationService::prepare(const Session& s, const std::string& appointmentId) const {
    auto a = ownScheduledAppointment(s, appointmentId);
    if (!a.ok()) return a.error();
    auto p = patients_.findById(a.value().patientId);
    if (!p) return makeError(ErrorCode::E020, "Patient not found.");
    return PatientSummary{p->patientId, p->fullName, ageOn(p->dob, now_())};
}

Result<std::string> ConsultationService::record(const Session& s, const std::string& appointmentId, const ConsultationInput& in) {
    auto a = ownScheduledAppointment(s, appointmentId);
    if (!a.ok()) return a.error();
    if (auto v = validate(in); !v.ok()) return v.error();
    char id[16]; std::snprintf(id, sizeof id, "C%06d", counter_ + 1);
    byId_[id] = Consultation{id, appointmentId, a.value().patientId, a.value().doctorId, now_(), in.diagnosis, in.notes, in.items, {}};
    if (auto w = persist(); !w.ok()) { byId_.erase(id); (void)persist(); return w.error(); }   // undo, best effort
    if (auto done = appts_.markCompleted(appointmentId); !done.ok()) {   // keep both files consistent
        byId_.erase(id);
        (void)persist();
        return done.error();
    }
    ++counter_;
    audit_.log(s.userId, "CONSULT_SAVE", id, true);                    // never the clinical text (HMS-SEC-003)
    return std::string(id);
}

Result<std::vector<Consultation>> ConsultationService::history(const Session&, const std::string&) { return notImplementedError(); }

Result<void> ConsultationService::addAddendum(const Session&, const std::string&, const std::string&) { return notImplementedError(); }

Result<void> ConsultationService::editConsultation(const Session&, const std::string&, const ConsultationInput&) { return notImplementedError(); }

Result<void> ConsultationService::deleteConsultation(const Session&, const std::string&) { return notImplementedError(); }
}  // namespace hms
