// Member 3 (SF-3) - TC-APT-04 atomic reschedule, TC-APT-05 daily schedule (HMS-F-033, HMS-F-034)
#include "services/appointment_service.h"
#include "test_helpers.h"
#include <filesystem>
#include <gtest/gtest.h>

using namespace hms;
using hms_test::session;

namespace {
struct MoveFixture : ::testing::Test {
    std::string dir = hms_test::freshDir("move");
    std::string now = "2026-10-26 08:00";
    AuditService audit{dir + "/audit.log"};
    PatientRepository patients;
    DoctorService doctors{audit, dir, [](const std::string&) { return true; }};
    AppointmentService appts{patients, doctors, audit, dir, [this] { return now; }};
    Session admin = session(Role::Admin), rec = session(Role::Receptionist, "U00002");
    std::string d1, d2;
    void SetUp() override {
        for (int i = 1; i <= 5; ++i)
            patients.put(Patient{"P00000" + std::to_string(i), "Patient " + std::string(1, char('A' + i)), "1990-01-01", "F",
                                 "987654321" + std::to_string(i), "MG Road", "B+", true, "2026-10-19"});
        d1 = doctors.addDoctor(admin, {"Meera Iyer", "Cardiology", "Cardiac", "U90001"}).value();
        d2 = doctors.addDoctor(admin, {"Ravi Shah", "Orthopaedics", "Ortho", "U90002"}).value();
        ASSERT_TRUE(doctors.setAvailability(admin, d1, {{1, "09:00", "13:00"}}).ok());
        ASSERT_TRUE(doctors.setAvailability(admin, d2, {{1, "09:00", "13:00"}}).ok());
    }
    void TearDown() override { std::filesystem::remove_all(dir); }
};
}  // namespace

// TC-APT-04: move A to a free slot X; moving to an occupied slot Y is refused and A stays in X
TEST_F(MoveFixture, TC_APT_04_AtomicReschedule) {
    const auto a = appts.book(rec, "P000001", d1, "2026-10-26 09:00").value();
    ASSERT_TRUE(appts.book(rec, "P000002", d2, "2026-10-26 11:00").ok());                 // Y is taken
    ASSERT_TRUE(appts.reschedule(rec, a, d2, "2026-10-26 10:00").ok());                  // X, other doctor
    EXPECT_EQ(appts.get(a)->doctorId, d2);
    EXPECT_EQ(appts.get(a)->slotStart, "2026-10-26 10:00");
    EXPECT_TRUE(appts.book(rec, "P000003", d1, "2026-10-26 09:00").ok());                 // old slot is free
    auto refused = appts.reschedule(rec, a, d2, "2026-10-26 11:00");
    ASSERT_FALSE(refused.ok());
    EXPECT_EQ(refused.error().code, ErrorCode::E030);
    EXPECT_EQ(appts.get(a)->slotStart, "2026-10-26 10:00");                              // unchanged
    EXPECT_EQ(appts.reschedule(rec, a, d2, "2026-10-26 10:00").error().code, ErrorCode::E010);   // same slot
    EXPECT_EQ(appts.reschedule(rec, a, "D999", "2026-10-26 12:00").error().code, ErrorCode::E021);
    EXPECT_EQ(appts.reschedule(session(Role::Doctor), a, d2, "2026-10-26 12:00").error().code, ErrorCode::E004);

    AppointmentService reloaded{patients, doctors, audit, dir, [this] { return now; }};
    ASSERT_TRUE(reloaded.load().ok());
    EXPECT_EQ(reloaded.get(a)->slotStart, "2026-10-26 10:00");
    EXPECT_EQ(reloaded.book(rec, "P000004", d2, "2026-10-26 10:00").error().code, ErrorCode::E030);
}

// TC-APT-05: sorted by time, includes all statuses; a Doctor sees only their own schedule
TEST_F(MoveFixture, TC_APT_05_DailySchedule) {
    std::string cancelled;
    for (const char* t : {"11:00", "09:30", "12:30", "10:00", "09:00"}) {
        auto id = appts.book(rec, "P000001", d1, std::string("2026-10-26 ") + t).value();
        if (std::string(t) == "10:00") cancelled = id;
    }
    ASSERT_TRUE(appts.cancel(rec, cancelled).ok());
    Session doc1 = session(Role::Doctor, "U90001"); doc1.doctorId = d1;
    Session doc2 = session(Role::Doctor, "U90002"); doc2.doctorId = d2;
    auto mine = appts.schedule(doc1, d1, "2026-10-26");
    ASSERT_TRUE(mine.ok());
    ASSERT_EQ(mine.value().size(), 5u);
    for (size_t i = 1; i < mine.value().size(); ++i) EXPECT_LT(mine.value()[i - 1].slotStart, mine.value()[i].slotStart);
    EXPECT_EQ(mine.value()[2].status, AppointmentStatus::Cancelled);
    EXPECT_EQ(appts.schedule(doc2, d1, "2026-10-26").error().code, ErrorCode::E004);
    EXPECT_EQ(appts.schedule(rec, d1, "2026-10-26").value().size(), 5u);
    EXPECT_TRUE(appts.schedule(rec, d1, "2026-10-27").value().empty());
    EXPECT_EQ(appts.schedule(rec, d1, "2026-13-01").error().code, ErrorCode::E010);
}
