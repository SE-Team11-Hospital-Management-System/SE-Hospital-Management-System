// SPRINT 2 PLACEHOLDER. Replaced by the owner's pull request this week - see the header for the owner.
// Owner: Member 4 (Shristi).
#include "services/consultation_service.h"

namespace hms {
ConsultationService::ConsultationService(AppointmentService& appts, PatientRepository& patients, AuditService& audit,
                                         std::string dataDir, NowFn now)
    : appts_(appts), patients_(patients), audit_(audit), dir_(std::move(dataDir)), now_(std::move(now)) {}
Result<void> ConsultationService::load() {
    (void)appts_; (void)patients_; (void)audit_; (void)dir_; (void)now_; (void)counter_; byId_.clear();
    return {};
}
Result<void> ConsultationService::persist() const { return notImplementedError(); }
Result<Appointment> ConsultationService::ownScheduledAppointment(const Session&, const std::string&) const { return notImplementedError(); }
Result<void> ConsultationService::validate(const ConsultationInput&) { return notImplementedError(); }
int ConsultationService::ageOn(const std::string&, const std::string&) { return 0; }
Result<PatientSummary> ConsultationService::prepare(const Session&, const std::string&) const { return notImplementedError(); }
Result<std::string> ConsultationService::record(const Session&, const std::string&, const ConsultationInput&) { return notImplementedError(); }
Result<std::vector<Consultation>> ConsultationService::history(const Session&, const std::string&) { return notImplementedError(); }
Result<void> ConsultationService::addAddendum(const Session&, const std::string&, const std::string&) { return notImplementedError(); }
Result<void> ConsultationService::editConsultation(const Session&, const std::string&, const ConsultationInput&) { return notImplementedError(); }
Result<void> ConsultationService::deleteConsultation(const Session&, const std::string&) { return notImplementedError(); }
}  // namespace hms
