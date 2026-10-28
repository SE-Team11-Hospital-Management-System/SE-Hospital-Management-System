#include "services/report_service.h"
#include "common/datetime.h"
#include "data/file_store.h"
#include "security/access_control.h"
#include "security/validator.h"
#include <algorithm>
#include <filesystem>
#include <fstream>

namespace hms {

namespace {
std::string csvField(const std::string& s) {                            // quote fields that need it (RFC 4180)
    if (s.find_first_of(",\"") == std::string::npos) return s;
    std::string out = "\"";
    for (char c : s) out += (c == '"') ? std::string("\"\"") : std::string(1, c);
    return out + "\"";
}
}  // namespace

ReportService::ReportService(AppointmentService& appts, DoctorService& doctors, PatientRepository& patients, std::string reportsDir)
    : appts_(appts), doctors_(doctors), patients_(patients), dir_(std::move(reportsDir)) {}

Result<DailyReport> ReportService::daily(const Session& s, const std::string& date) const {
    if (auto ok = AccessControl::authorize(s, Operation::DailyReport, "dailyReport"); !ok.ok()) return ok.error();
    if (!Validator::isValidDate(date)) return makeError(ErrorCode::E010, "Date: must be a real date in YYYY-MM-DD format.");
    DailyReport r;
    r.date = date;
    for (auto st : {AppointmentStatus::Scheduled, AppointmentStatus::Completed, AppointmentStatus::Cancelled, AppointmentStatus::NoShow})
        r.perStatus[st] = 0;
    for (const auto& a : appts_.all()) {
        if (dt::dateOf(a.slotStart) != date) continue;
        ++r.total;
        ++r.perStatus[a.status];
        r.perDoctor[a.doctorId].push_back(a);
    }
    for (auto& [doc, list] : r.perDoctor)
        std::sort(list.begin(), list.end(), [](const Appointment& x, const Appointment& y) { return x.slotStart < y.slotStart; });
    return r;
}

Result<std::string> ReportService::exportCsv(const Session& s, const DailyReport& r) const {
    if (auto ok = AccessControl::authorize(s, Operation::DailyReport, "exportReport"); !ok.ok()) return ok.error();
    if (!FileStore::ensureDirectory(dir_).ok()) return writeFailedError();
    const std::string path = dir_ + "/daily_" + r.date + ".csv";
    std::ofstream out(path, std::ios::trunc);
    if (!out) return writeFailedError();
    out << "appointment_id,date,time,doctor_id,doctor_name,patient_id,patient_name,status\n";
    for (const auto& [doc, list] : r.perDoctor) {
        const auto d = doctors_.get(doc);
        for (const auto& a : list) {
            const auto p = patients_.findById(a.patientId);
            out << a.appointmentId << ',' << r.date << ',' << dt::timeOf(a.slotStart) << ',' << doc << ','
                << csvField(d ? d->fullName : "") << ',' << a.patientId << ',' << csvField(p ? p->fullName : "") << ','
                << toString(a.status) << '\n';
        }
    }
    out.close();
    if (!out) return writeFailedError();
    std::error_code ec;                                                  // contains patient names: owner only (HMS-SEC-006)
    std::filesystem::permissions(path, std::filesystem::perms::owner_read | std::filesystem::perms::owner_write,
                                 std::filesystem::perm_options::replace, ec);
    return path;
}

Result<std::vector<WorkloadRow>> ReportService::workload(const Session& s, const std::string& from, const std::string& to) const {
    if (auto ok = AccessControl::authorize(s, Operation::WorkloadReport, "workloadReport"); !ok.ok()) return ok.error();
    if (!Validator::isValidDate(from)) return makeError(ErrorCode::E010, "Start date: must be a real date in YYYY-MM-DD format.");
    if (!Validator::isValidDate(to)) return makeError(ErrorCode::E010, "End date: must be a real date in YYYY-MM-DD format.");
    if (to < from) return makeError(ErrorCode::E010, "Date range: the end date must not be before the start date.");
    auto list = doctors_.list(s);
    if (!list.ok()) return list.error();
    std::map<std::string, WorkloadRow> rows;
    for (const auto& d : list.value()) rows[d.doctorId] = WorkloadRow{d.doctorId, d.fullName, 0};
    for (const auto& a : appts_.all()) {
        const std::string day = dt::dateOf(a.slotStart);
        if (a.status == AppointmentStatus::Completed && day >= from && day <= to && rows.count(a.doctorId)) ++rows[a.doctorId].completed;
    }
    std::vector<WorkloadRow> out;
    for (const auto& [id, row] : rows) out.push_back(row);
    return out;
}
}  // namespace hms
