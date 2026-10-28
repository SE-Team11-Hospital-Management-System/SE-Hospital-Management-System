// Member 4 (SF-4) - TC-RPT-01 daily report + CSV, TC-RPT-02 workload (HMS-F-050, HMS-F-051)
#include "services/report_service.h"
#include "test_helpers.h"
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

using namespace hms;
using hms_test::session;

namespace {
struct ReportFixture : ::testing::Test {
    std::string dir = hms_test::freshDir("report");
    std::string now = "2026-10-26 07:00";
    AuditService audit{dir + "/audit.log"};
    PatientRepository patients;
    DoctorService doctors{audit, dir, [](const std::string&) { return true; }};
    AppointmentService appts{patients, doctors, audit, dir, [this] { return now; }};
    ReportService reports{appts, doctors, patients, dir + "/reports"};
    Session admin = session(Role::Admin), rec = session(Role::Receptionist, "U00002");
    std::string d1, d2;
    void SetUp() override {
        patients.put(Patient{"P000001", "Rao, Anita", "1990-05-14", "F", "9876543210", "MG Road", "B+", true, "2026-10-19"});
        d1 = doctors.addDoctor(admin, {"Meera Iyer", "Cardiology", "Cardiac", "U90001"}).value();
        d2 = doctors.addDoctor(admin, {"Ravi Shah", "Orthopaedics", "Ortho", "U90002"}).value();
        std::vector<Window> all;
        for (int day = 0; day < 7; ++day) all.push_back(Window{day, "08:00", "20:00"});
        ASSERT_TRUE(doctors.setAvailability(admin, d1, all).ok());
        ASSERT_TRUE(doctors.setAvailability(admin, d2, all).ok());
    }
    std::string book(const std::string& d, const std::string& slot) { return appts.book(rec, "P000001", d, slot).value(); }
    void TearDown() override { std::filesystem::remove_all(dir); }
};
}  // namespace

// TC-RPT-01: 10 appointments (6 Scheduled, 2 Completed, 1 Cancelled, 1 No-Show); CSV has a header and 10 rows
TEST_F(ReportFixture, TC_RPT_01_DailyReportAndCsv) {
    std::vector<std::string> ids;
    for (int i = 0; i < 10; ++i) {
        char slot[20]; std::snprintf(slot, sizeof slot, "2026-10-26 %02d:%s", 9 + i / 2, i % 2 ? "30" : "00");
        ids.push_back(book(i < 5 ? d1 : d2, slot));
    }
    ASSERT_TRUE(appts.markCompleted(ids[0]).ok());
    ASSERT_TRUE(appts.markCompleted(ids[1]).ok());
    ASSERT_TRUE(appts.cancel(rec, ids[9]).ok());
    now = "2026-10-26 11:00";
    ASSERT_TRUE(appts.markNoShow(rec, ids[2]).ok());
    book(d1, "2026-10-27 09:00");                                                 // another day: not counted

    auto r = reports.daily(rec, "2026-10-26");
    ASSERT_TRUE(r.ok());
    EXPECT_EQ(r.value().total, 10);
    EXPECT_EQ(r.value().perStatus.at(AppointmentStatus::Scheduled), 6);
    EXPECT_EQ(r.value().perStatus.at(AppointmentStatus::Completed), 2);
    EXPECT_EQ(r.value().perStatus.at(AppointmentStatus::Cancelled), 1);
    EXPECT_EQ(r.value().perStatus.at(AppointmentStatus::NoShow), 1);
    EXPECT_EQ(r.value().perDoctor.at(d1).size(), 5u);

    auto path = reports.exportCsv(rec, r.value());
    ASSERT_TRUE(path.ok());
    std::ifstream in(path.value());
    std::vector<std::string> lines; std::string line;
    while (std::getline(in, line)) lines.push_back(line);
    ASSERT_EQ(lines.size(), 11u);
    EXPECT_EQ(lines[0], "appointment_id,date,time,doctor_id,doctor_name,patient_id,patient_name,status");
    EXPECT_NE(lines[1].find("\"Rao, Anita\""), std::string::npos);              // comma in a name is quoted
    EXPECT_EQ(reports.daily(session(Role::Doctor), "2026-10-26").error().code, ErrorCode::E004);
    EXPECT_EQ(reports.daily(rec, "2026-02-30").error().code, ErrorCode::E010);
}

// TC-RPT-02: Completed per doctor over a range; a reversed range is rejected
TEST_F(ReportFixture, TC_RPT_02_Workload) {
    const auto a = book(d1, "2026-10-26 09:00"), b = book(d1, "2026-11-02 09:00"), c = book(d2, "2026-11-03 09:00");
    book(d2, "2026-11-04 09:00");                                                 // stays Scheduled: not counted
    const auto outside = book(d1, "2026-11-20 09:00");
    for (const auto& id : {a, b, c, outside}) ASSERT_TRUE(appts.markCompleted(id).ok());
    auto r = reports.workload(admin, "2026-10-26", "2026-11-08");
    ASSERT_TRUE(r.ok());
    ASSERT_EQ(r.value().size(), 2u);
    EXPECT_EQ(r.value()[0].doctorId, d1);
    EXPECT_EQ(r.value()[0].completed, 2);
    EXPECT_EQ(r.value()[1].completed, 1);
    EXPECT_EQ(reports.workload(admin, "2026-11-08", "2026-10-26").error().code, ErrorCode::E010);
    EXPECT_EQ(reports.workload(rec, "2026-10-26", "2026-11-08").error().code, ErrorCode::E004);
}
