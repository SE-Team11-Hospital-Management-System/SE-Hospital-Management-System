// Member 3 (SF-3) - TC-APT-01, TC-APT-02 (HMS-F-030/031)
#include "services/appointment_service.h"
#include "test_helpers.h"
#include <filesystem>
#include <gtest/gtest.h>

using namespace hms;
using hms_test::session;

namespace {
struct AppointmentFixture : ::testing::Test {
    std::string dir = hms_test::freshDir("doc");
    std::string now = "2026-10-19 08:00";                                // Monday morning
    AuditService audit{dir + "/audit.log"};
    PatientRepository patients;
    DoctorService doctors{audit, dir, [](const std::string& u) { return u.rfind("U9", 0) == 0; }};   // U9xxxx = doctor accounts in tests
    AppointmentService appts{patients, doctors, audit, dir, [this] { return now; }};
    Session admin = session(Role::Admin), rec = session(Role::Receptionist, "U00002");
    std::string d1;
    void SetUp() override {
        patients.put(Patient{"P000001", "Anita Rao", "1990-05-14", "F", "9876543210", "MG Road", "B+", true, "2026-10-19"});
        patients.put(Patient{"P000002", "Old Patient", "1950-01-01", "M", "9876543211", "MG Road", "O+", false, "2026-10-19"});
        d1 = doctors.addDoctor(admin, {"Meera Iyer", "Cardiology", "Cardiac Sciences", "U90001"}).value();
        ASSERT_TRUE(doctors.setAvailability(admin, d1, {{1, "09:00", "11:00"}}).ok());   // Mon 09:00-11:00 = 4 slots
    }
    void TearDown() override { std::filesystem::remove_all(dir); }
};
}  // namespace

// TC-APT-01: valid booking gets A00000001, status SCHEDULED, saved to disk
TEST_F(AppointmentFixture, TC_APT_01_BookValidSlot) {
    auto r = appts.book(rec, "P000001", d1, "2026-10-19 09:00");
    ASSERT_TRUE(r.ok()) << r.error().message;
    EXPECT_EQ(r.value(), "A00000001");
    EXPECT_EQ(appts.get(r.value())->status, AppointmentStatus::Scheduled);

    AppointmentService reloaded{patients, doctors, audit, dir, [this] { return now; }};
    ASSERT_TRUE(reloaded.load().ok());
    EXPECT_TRUE(reloaded.get("A00000001").has_value());
    EXPECT_EQ(reloaded.book(rec, "P000001", d1, "2026-10-19 09:00").error().code, ErrorCode::E030);   // conflict survives restart
}

// TC-APT-02: taken / past / outside availability rejected with reason + next 3 free slots
TEST_F(AppointmentFixture, TC_APT_02_ConflictsAndAlternatives) {
    ASSERT_TRUE(appts.book(rec, "P000001", d1, "2026-10-19 09:00").ok());
    auto taken = appts.book(rec, "P000001", d1, "2026-10-19 09:00");
    ASSERT_FALSE(taken.ok());
    EXPECT_EQ(taken.error().code, ErrorCode::E030);
    EXPECT_EQ(taken.error().details, "2026-10-19 09:30, 2026-10-19 10:00, 2026-10-19 10:30");

    now = "2026-10-19 10:00";
    auto past = appts.book(rec, "P000001", d1, "2026-10-19 09:30");
    EXPECT_EQ(past.error().code, ErrorCode::E031);
    EXPECT_EQ(past.error().details.substr(0, 16), "2026-10-19 10:30");

    auto outside = appts.book(rec, "P000001", d1, "2026-10-20 09:00");     // Tuesday: no availability
    EXPECT_EQ(outside.error().code, ErrorCode::E031);
    EXPECT_EQ(appts.book(rec, "P000002", d1, "2026-10-26 09:00").error().code, ErrorCode::E025);   // inactive patient
    EXPECT_EQ(appts.book(rec, "P000009", d1, "2026-10-26 09:00").error().code, ErrorCode::E020);
    EXPECT_EQ(appts.book(rec, "P000001", "D999", "2026-10-26 09:00").error().code, ErrorCode::E021);
    EXPECT_EQ(appts.book(rec, "P000001", d1, "2026-10-26 09:15").error().code, ErrorCode::E010);
    EXPECT_EQ(appts.book(session(Role::Doctor), "P000001", d1, "2026-10-26 09:00").error().code, ErrorCode::E004);
    EXPECT_EQ(appts.all().size(), 1u);
}
