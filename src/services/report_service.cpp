// SPRINT 2 PLACEHOLDER. Replaced by the owner's pull request this week - see the header for the owner.
// Owner: Member 4 (Shristi).
#include "services/report_service.h"

namespace hms {
ReportService::ReportService(AppointmentService& appts, DoctorService& doctors, PatientRepository& patients, std::string reportsDir)
    : appts_(appts), doctors_(doctors), patients_(patients), dir_(std::move(reportsDir)) {}
Result<DailyReport> ReportService::daily(const Session&, const std::string&) const {
    (void)appts_; (void)doctors_; (void)patients_; (void)dir_;
    return notImplementedError();
}
Result<std::string> ReportService::exportCsv(const Session&, const DailyReport&) const { return notImplementedError(); }
Result<std::vector<WorkloadRow>> ReportService::workload(const Session&, const std::string&, const std::string&) const { return notImplementedError(); }
}  // namespace hms
