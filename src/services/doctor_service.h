// C5 DoctorService: doctor profiles and weekly availability in 30-minute slots (HMS-F-020/021/022).
// Saved to <dataDir>/doctors.dat, one doctor per line:
//   doctorId|fullName|specialization|department|userId|windows   where windows = "1@09:00-13:00;3@14:00-17:00"
#pragma once
#include "common/types.h"
#include "security/audit_service.h"
#include "security/session_manager.h"
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace hms {

struct Window { int weekday; std::string start, end; };           // weekday 0 = Sunday ... 6 = Saturday; "HH:MM"
struct DoctorInput { std::string fullName, specialization, department, userId; };
struct Doctor { std::string doctorId, fullName, specialization, department, userId; std::vector<Window> windows; };

class DoctorService {
public:
    using IsDoctorUserFn = std::function<bool(const std::string&)>;  // true if userId is an existing Doctor account
    DoctorService(AuditService& audit, std::string dataDir, IsDoctorUserFn isDoctorUser);

    Result<void> load();
    Result<std::string> addDoctor(const Session& s, const DoctorInput& in);                         // D001, D002 ...
    Result<void> updateDoctor(const Session& s, const std::string& doctorId, const DoctorInput& changes);   // empty field = keep
    Result<void> setAvailability(const Session& s, const std::string& doctorId, const std::vector<Window>& w);
    Result<std::vector<Doctor>> list(const Session& s, const std::optional<std::string>& specialization = std::nullopt) const;

    std::optional<Doctor> get(const std::string& doctorId) const;
    bool exists(const std::string& doctorId) const { return doctors_.count(doctorId) > 0; }
    size_t count() const { return doctors_.size(); }
    bool isWithinAvailability(const std::string& doctorId, const std::string& slotStart) const;   // slot = "YYYY-MM-DD HH:MM"
    static bool isValidSlotTime(const std::string& hhmm);             // 08:00-20:00 on a 30-minute boundary
private:
    Result<void> validateInput(const DoctorInput& in) const;
    Result<void> persist() const;
    [[maybe_unused]] AuditService& audit_;
    std::string path_;
    IsDoctorUserFn isDoctorUser_;
    std::map<std::string, Doctor> doctors_;
    [[maybe_unused]] int counter_{0};
};
}  // namespace hms
