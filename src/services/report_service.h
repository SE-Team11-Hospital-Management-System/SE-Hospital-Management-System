// C8 ReportService: daily appointment report with CSV export, doctor workload (HMS-F-050, HMS-F-051).
// Reports never contain clinical data (HMS-F-042).
#pragma once
#include "common/types.h"
#include "data/patient_repository.h"
#include "security/session_manager.h"
#include "services/appointment_service.h"
#include "services/doctor_service.h"
#include <map>
#include <string>
#include <vector>

namespace hms {

struct DailyReport {
    std::string date;                                           // YYYY-MM-DD
    int total{0};
    std::map<AppointmentStatus, int> perStatus;                 // all four statuses, zero if none
    std::map<std::string, std::vector<Appointment>> perDoctor;  // doctorId -> appointments sorted by time
};

struct WorkloadRow { std::string doctorId, doctorName; int completed{0}; };

class ReportService {
public:
    ReportService(AppointmentService& appts, DoctorService& doctors, PatientRepository& patients, std::string reportsDir);
    Result<DailyReport> daily(const Session& s, const std::string& date) const;
    // Writes <reportsDir>/daily_<date>.csv and returns its path.
    Result<std::string> exportCsv(const Session& s, const DailyReport& r) const;
    Result<std::vector<WorkloadRow>> workload(const Session& s, const std::string& from, const std::string& to) const;
private:
    AppointmentService& appts_;
    DoctorService& doctors_;
    PatientRepository& patients_;
    std::string dir_;
};
}  // namespace hms
