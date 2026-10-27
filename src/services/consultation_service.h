// C7 ConsultationService: clinical records, Doctors only (HMS-F-040..043, HMS-NF-010).
// Saved records are never edited or deleted; a Doctor may only add a time-stamped addendum.
// Files in <dataDir>:
//   consultations.dat  consultId|appointmentId|patientId|doctorId|createdAt|diagnosis|notes
//   prescriptions.dat  consultId|itemNo|medicine|dosage|days
//   addenda.dat        consultId|timestamp|authorUserId|text
#pragma once
#include "common/types.h"
#include "data/patient_repository.h"
#include "security/audit_service.h"
#include "security/session_manager.h"
#include "services/appointment_service.h"
#include "services/doctor_service.h"
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace hms {

struct PrescriptionItem { std::string medicine, dosage; int days{0}; };
struct Addendum { std::string timestamp, authorUserId, text; };

struct ConsultationInput {
    std::string diagnosis;                       // 1-500 characters
    std::vector<PrescriptionItem> items;         // 0-10 items
    std::string notes;                           // 0-1000 characters
};

struct Consultation {
    std::string consultId, appointmentId, patientId, doctorId, createdAt, diagnosis, notes;
    std::vector<PrescriptionItem> items;
    std::vector<Addendum> addenda;
};

struct PatientSummary { std::string patientId, fullName; int age{0}; };   // shown before saving (HMS-NF-010)

class ConsultationService {
public:
    using NowFn = std::function<std::string()>;  // "YYYY-MM-DD HH:MM"
    ConsultationService(AppointmentService& appts, PatientRepository& patients, AuditService& audit,
                        std::string dataDir, NowFn now);

    Result<void> load();
    // Checks that the Doctor may record for this appointment and returns who the patient is.
    Result<PatientSummary> prepare(const Session& s, const std::string& appointmentId) const;
    // Saves the consultation and marks the appointment Completed. Returns C000001, C000002 ...
    Result<std::string> record(const Session& s, const std::string& appointmentId, const ConsultationInput& in);
    // All consultations of a patient, newest first. Each call is audit-logged (CONSULT_VIEW).
    Result<std::vector<Consultation>> history(const Session& s, const std::string& patientId);
    Result<void> addAddendum(const Session& s, const std::string& consultId, const std::string& text);
    // HMS-F-043: saved consultations can never be changed. These always refuse (E004 or E032).
    Result<void> editConsultation(const Session& s, const std::string& consultId, const ConsultationInput& in);
    Result<void> deleteConsultation(const Session& s, const std::string& consultId);

    static Result<void> validate(const ConsultationInput& in);
    static int ageOn(const std::string& dob, const std::string& today);    // full years
    static constexpr size_t kMaxItems = 10, kMaxDiagnosis = 500, kMaxNotes = 1000;
private:
    Result<Appointment> ownScheduledAppointment(const Session& s, const std::string& appointmentId) const;
    Result<void> persist() const;
    AppointmentService& appts_;
    PatientRepository& patients_;
    AuditService& audit_;
    std::string dir_;
    NowFn now_;
    std::map<std::string, Consultation> byId_;
    int counter_{0};
};
}  // namespace hms
