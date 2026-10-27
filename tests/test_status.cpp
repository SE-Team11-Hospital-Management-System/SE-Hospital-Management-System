// Member 3 (SF-3) - TC-APT-03 cancel, TC-APT-06 status transitions (HMS-F-032, HMS-F-035)
#include "services/appointment_service.h"
#include "test_helpers.h"
#include <filesystem>
#include <gtest/gtest.h>

using namespace hms;
using hms_test::session;

namespace {
struct StatusFixture : ::testing::Test {
    std::string dir = hms_test::freshDir("status");
    std::string now = "2026-10-26 08:00";                                     // Monday morning
    AuditService audit{dir + "/audit.log"};
    PatientRepository patients;
    DoctorService doctors{audit, dir, [](const std::string&) { return true; }};
    AppointmentService appts{patients, doctors, audit, dir, [this] { return now; }};
    Session admin = session(Role::Admin), rec = session(Role::Receptionist, "U00002");
    std::string d1;
    void SetUp() override {
        patients.put(Patient{"P000001", "Anita Rao", "1990-05-14", "F", "9876543210", "MG Road", "B+", true, "2026-10-19"});
        patients.put(Patient{"P000002", "Anil Kumar", "1985-01-01", "M", "9123456780", "MG Road", "O+", true, "2026-10-19"});
        d1 = doctors.addDoctor(admin, {"Meera Iyer", "Cardiology", "Cardiac", "U90001"}).value();
        ASSERT_TRUE(doctors.setAvailability(admin, d1, {{1, "09:00", "13:00"}}).ok());
    }
    void TearDown() override { std::filesystem::remove_all(dir); }
};
}  // namespace

// TC-APT-03: cancel frees the slot at once; a Completed appointment cannot be cancelled
TEST_F(StatusFixture, TC_APT_03_CancelFreesSlot) {
    const auto a1 = appts.book(rec, "P000001", d1, "2026-10-26 09:00").value();
    const auto a2 = appts.book(rec, "P000002", d1, "2026-10-26 09:30").value();
    ASSERT_TRUE(appts.markCompleted(a2).ok());
    ASSERT_TRUE(appts.cancel(rec, a1).ok());
    EXPECT_EQ(appts.get(a1)->status, AppointmentStatus::Cancelled);
    EXPECT_TRUE(appts.book(rec, "P000002", d1, "2026-10-26 09:00").ok());     // freed slot is bookable again
    auto refused = appts.cancel(rec, a2);
    ASSERT_FALSE(refused.ok());
    EXPECT_EQ(refused.error().code, ErrorCode::E032);
    EXPECT_EQ(appts.cancel(session(Role::Doctor), a1).error().code, ErrorCode::E004);
    EXPECT_EQ(appts.cancel(rec, "A99999999").error().code, ErrorCode::E022);

    AppointmentService reloaded{patients, doctors, audit, dir, [this] { return now; }};   // saved to disk
    ASSERT_TRUE(reloaded.load().ok());
    EXPECT_EQ(reloaded.get(a1)->status, AppointmentStatus::Cancelled);
    EXPECT_EQ(reloaded.get(a2)->status, AppointmentStatus::Completed);
}

// TC-APT-06: only Scheduled -> Completed / Cancelled / No-Show; No-Show only after the start time
TEST_F(StatusFixture, TC_APT_06_StatusTransitions) {
    const auto past = appts.book(rec, "P000001", d1, "2026-10-26 09:00").value();
    const auto future = appts.book(rec, "P000002", d1, "2026-10-26 12:00").value();
    const auto cancelled = appts.book(rec, "P000001", d1, "2026-10-26 10:00").value();
    const auto done = appts.book(rec, "P000002", d1, "2026-10-26 10:30").value();
    ASSERT_TRUE(appts.cancel(rec, cancelled).ok());
    ASSERT_TRUE(appts.markCompleted(done).ok());
    now = "2026-10-26 09:45";                                                 // 'past' has started, 'future' has not
    EXPECT_TRUE(appts.markNoShow(rec, past).ok());                            // step 1
    EXPECT_EQ(appts.changeStatus(rec, cancelled, AppointmentStatus::Scheduled).error().code, ErrorCode::E032);   // step 2
    EXPECT_EQ(appts.cancel(rec, done).error().code, ErrorCode::E032);                                           // step 3
    EXPECT_EQ(appts.markNoShow(rec, future).error().code, ErrorCode::E032);                                     // step 4
    EXPECT_EQ(appts.cancel(rec, past).error().code, ErrorCode::E032);         // No-Show is final
    EXPECT_EQ(appts.changeStatus(rec, future, AppointmentStatus::Completed).error().code, ErrorCode::E032);    // only via consultation
    EXPECT_TRUE(AppointmentService::canChange(AppointmentStatus::Scheduled, AppointmentStatus::Cancelled));
    EXPECT_FALSE(AppointmentService::canChange(AppointmentStatus::Completed, AppointmentStatus::Cancelled));
}
