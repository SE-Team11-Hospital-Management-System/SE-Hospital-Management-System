// Member 3 (SF-3) - TC-DOC-01 (HMS-F-020/021/022)
#include "services/doctor_service.h"
#include "test_helpers.h"
#include "data/patient_repository.h"
#include <filesystem>
#include <gtest/gtest.h>

using namespace hms;
using hms_test::session;

namespace {
struct DoctorFixture : ::testing::Test {
    std::string dir = hms_test::freshDir("doc");
    AuditService audit{dir + "/audit.log"};
    PatientRepository patients;
    DoctorService doctors{audit, dir, [](const std::string& u) { return u.rfind("U9", 0) == 0; }};   // U9xxxx = doctor accounts in tests
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

// TC-DOC-01: add / update profile, availability rules, list with filter, saved to disk
TEST_F(DoctorFixture, TC_DOC_01_ProfilesAvailabilityAndList) {
    EXPECT_EQ(d1, "D001");
    auto d2 = doctors.addDoctor(admin, {"Ravi Shah", "Orthopaedics", "Ortho", "U90002"});
    ASSERT_TRUE(d2.ok());
    EXPECT_EQ(d2.value(), "D002");
    EXPECT_EQ(doctors.addDoctor(admin, {"No Account", "ENT", "ENT", "U00002"}).error().code, ErrorCode::E023);
    EXPECT_EQ(doctors.addDoctor(rec, {"Rec Tries", "ENT", "ENT", "U90003"}).error().code, ErrorCode::E004);
    EXPECT_EQ(doctors.setAvailability(admin, d1, {{1, "09:15", "11:00"}}).error().code, ErrorCode::E010);   // not on 30-min boundary
    EXPECT_EQ(doctors.setAvailability(admin, d1, {{1, "07:00", "11:00"}}).error().code, ErrorCode::E010);   // before 08:00
    EXPECT_EQ(doctors.setAvailability(admin, d1, {{1, "11:00", "09:00"}}).error().code, ErrorCode::E010);   // start after end
    ASSERT_TRUE(doctors.updateDoctor(admin, d1, {"", "Cardiology", "Heart Centre", ""}).ok());
    EXPECT_EQ(doctors.get(d1)->department, "Heart Centre");
    EXPECT_EQ(doctors.list(rec).value().size(), 2u);
    EXPECT_EQ(doctors.list(session(Role::Doctor), std::string("Cardiology")).value().size(), 1u);

    DoctorService reloaded{audit, dir, [](const std::string&) { return true; }};
    ASSERT_TRUE(reloaded.load().ok());
    ASSERT_TRUE(reloaded.get(d1).has_value());
    EXPECT_EQ(reloaded.get(d1)->windows.size(), 1u);
    EXPECT_TRUE(reloaded.isWithinAvailability(d1, "2026-10-26 10:30"));
}
