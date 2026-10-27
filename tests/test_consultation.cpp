// Member 4 (SF-4) - TC-CON-01 record consultation with patient confirmation (HMS-F-040, HMS-NF-010)
#include "services/consultation_service.h"
#include "test_helpers.h"
#include <filesystem>
#include <gtest/gtest.h>

using namespace hms;
using hms_test::session;

namespace {
struct ConsultFixture : ::testing::Test {
    std::string dir = hms_test::freshDir("consult");
    std::string now = "2026-10-26 09:10";
    AuditService audit{dir + "/audit.log"};
    PatientRepository patients;
    DoctorService doctors{audit, dir, [](const std::string&) { return true; }};
    AppointmentService appts{patients, doctors, audit, dir, [this] { return now; }};
    ConsultationService consults{appts, patients, audit, dir, [this] { return now; }};
    Session admin = session(Role::Admin), rec = session(Role::Receptionist, "U00002");
    Session doc1 = session(Role::Doctor, "U90001"), doc2 = session(Role::Doctor, "U90002");
    std::string appt;
    ConsultationInput good{"Hypertension", {{"Amlodipine", "5 mg once a day", 30}, {"Aspirin", "75 mg once a day", 30}}, "Review in 4 weeks."};
    void SetUp() override {
        patients.put(Patient{"P000001", "Anita Rao", "1990-05-14", "F", "9876543210", "MG Road", "B+", true, "2026-10-19"});
        doc1.doctorId = doctors.addDoctor(admin, {"Meera Iyer", "Cardiology", "Cardiac", "U90001"}).value();
        doc2.doctorId = doctors.addDoctor(admin, {"Ravi Shah", "Orthopaedics", "Ortho", "U90002"}).value();
        ASSERT_TRUE(doctors.setAvailability(admin, doc1.doctorId, {{1, "09:00", "13:00"}}).ok());
        now = "2026-10-26 08:00";
        appt = appts.book(rec, "P000001", doc1.doctorId, "2026-10-26 09:00").value();
        now = "2026-10-26 09:10";
    }
    void TearDown() override { std::filesystem::remove_all(dir); }
};
}  // namespace

// TC-CON-01: the patient is shown for confirmation; the valid record is saved and the appointment becomes Completed
TEST_F(ConsultFixture, TC_CON_01_RecordConsultation) {
    auto who = consults.prepare(doc1, appt);                                  // shown before saving (HMS-NF-010)
    ASSERT_TRUE(who.ok());
    EXPECT_EQ(who.value().patientId, "P000001");
    EXPECT_EQ(who.value().fullName, "Anita Rao");
    EXPECT_EQ(who.value().age, 36);

    ConsultationInput eleven = good;
    eleven.items.assign(11, PrescriptionItem{"Paracetamol", "500 mg", 3});
    EXPECT_EQ(consults.record(doc1, appt, eleven).error().code, ErrorCode::E010);
    ConsultationInput longDiagnosis = good;
    longDiagnosis.diagnosis = std::string(501, 'x');
    EXPECT_EQ(consults.record(doc1, appt, longDiagnosis).error().code, ErrorCode::E010);
    EXPECT_EQ(consults.record(doc2, appt, good).error().code, ErrorCode::E004);   // another doctor's appointment
    EXPECT_EQ(consults.record(rec, appt, good).error().code, ErrorCode::E004);
    EXPECT_EQ(appts.get(appt)->status, AppointmentStatus::Scheduled);           // nothing saved so far

    auto id = consults.record(doc1, appt, good);
    ASSERT_TRUE(id.ok()) << id.error().message;
    EXPECT_EQ(id.value(), "C000001");
    EXPECT_EQ(appts.get(appt)->status, AppointmentStatus::Completed);
    EXPECT_EQ(consults.record(doc1, appt, good).error().code, ErrorCode::E032);   // already completed

    ConsultationService reloaded{appts, patients, audit, dir, [this] { return now; }};
    ASSERT_TRUE(reloaded.load().ok());
    EXPECT_TRUE(reloaded.prepare(doc1, appt).error().code == ErrorCode::E032);
}

TEST(ConsultationRules, ValidationAndAge) {
    EXPECT_EQ(ConsultationService::validate({"", {}, ""}).error().code, ErrorCode::E010);
    EXPECT_TRUE(ConsultationService::validate({"Flu", {}, ""}).ok());
    EXPECT_EQ(ConsultationService::validate({"Flu", {{"X", "1 tab", 0}}, ""}).error().code, ErrorCode::E010);
    EXPECT_EQ(ConsultationService::validate({"Flu", {}, std::string(1001, 'n')}).error().code, ErrorCode::E010);
    EXPECT_EQ(ConsultationService::ageOn("1990-05-14", "2026-05-13 10:00"), 35);
    EXPECT_EQ(ConsultationService::ageOn("1990-05-14", "2026-05-14 10:00"), 36);
}
