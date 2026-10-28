// Member 4 (SF-4) - TC-CON-02 history and clinical privacy, TC-CON-03 immutability and addendum
// (HMS-F-041, F-042, F-043)
#include "services/consultation_service.h"
#include "test_helpers.h"
#include <filesystem>
#include <gtest/gtest.h>

using namespace hms;
using hms_test::session;

namespace {
struct HistoryFixture : ::testing::Test {
    std::string dir = hms_test::freshDir("history");
    std::string now = "2026-10-26 08:00";
    AuditService audit{dir + "/audit.log"};
    PatientRepository patients;
    DoctorService doctors{audit, dir, [](const std::string&) { return true; }};
    AppointmentService appts{patients, doctors, audit, dir, [this] { return now; }};
    ConsultationService consults{appts, patients, audit, dir, [this] { return now; }};
    Session admin = session(Role::Admin), rec = session(Role::Receptionist, "U00002");
    Session doc1 = session(Role::Doctor, "U90001");
    std::vector<std::string> ids;
    void SetUp() override {
        patients.put(Patient{"P000001", "Anita Rao", "1990-05-14", "F", "9876543210", "MG Road", "B+", true, "2026-10-19"});
        doc1.doctorId = doctors.addDoctor(admin, {"Meera Iyer", "Cardiology", "Cardiac", "U90001"}).value();
        ASSERT_TRUE(doctors.setAvailability(admin, doc1.doctorId, {{1, "09:00", "13:00"}}).ok());
        const char* slots[] = {"2026-10-26 09:00", "2026-11-02 09:00", "2026-11-09 09:00"};
        const char* times[] = {"2026-10-26 09:10", "2026-11-02 09:10", "2026-11-09 09:10"};
        std::vector<std::string> appointments;
        for (const char* sl : slots) appointments.push_back(appts.book(rec, "P000001", doc1.doctorId, sl).value());
        for (int i = 0; i < 3; ++i) {
            now = times[i];
            ids.push_back(consults.record(doc1, appointments[static_cast<size_t>(i)], {"Visit " + std::to_string(i + 1), {{"Drug", "1 tab", 5}}, ""}).value());
        }
    }
    void TearDown() override { std::filesystem::remove_all(dir); }
};
}  // namespace

// TC-CON-02: Doctor sees 3 entries newest first; Receptionist and Administrator are refused; view is audit-logged
TEST_F(HistoryFixture, TC_CON_02_HistoryAndPrivacy) {
    auto h = consults.history(doc1, "P000001");
    ASSERT_TRUE(h.ok());
    ASSERT_EQ(h.value().size(), 3u);
    EXPECT_EQ(h.value()[0].diagnosis, "Visit 3");
    EXPECT_EQ(h.value()[2].diagnosis, "Visit 1");
    EXPECT_EQ(h.value()[0].items.size(), 1u);
    EXPECT_EQ(consults.history(rec, "P000001").error().code, ErrorCode::E004);
    EXPECT_EQ(consults.history(admin, "P000001").error().code, ErrorCode::E004);
    EXPECT_EQ(consults.history(doc1, "P999999").error().code, ErrorCode::E020);
    bool viewed = false;
    for (const auto& e : audit.readAll()) viewed |= (e.action == "CONSULT_VIEW" && e.userId == "U90001" && e.targetId == "P000001");
    EXPECT_TRUE(viewed);
}

// TC-CON-03: no edit or delete; an addendum appears under the unchanged original with time and author
TEST_F(HistoryFixture, TC_CON_03_ImmutableWithAddendum) {
    EXPECT_EQ(consults.editConsultation(doc1, ids[0], {"Changed", {}, ""}).error().code, ErrorCode::E032);
    EXPECT_EQ(consults.deleteConsultation(doc1, ids[0]).error().code, ErrorCode::E032);
    EXPECT_EQ(consults.deleteConsultation(rec, ids[0]).error().code, ErrorCode::E004);
    now = "2026-11-10 10:00";
    ASSERT_TRUE(consults.addAddendum(doc1, ids[0], "Dose changed to 5 mg").ok());
    EXPECT_EQ(consults.addAddendum(doc1, "C999999", "x").error().code, ErrorCode::E024);
    EXPECT_EQ(consults.addAddendum(rec, ids[0], "x").error().code, ErrorCode::E004);

    ConsultationService reloaded{appts, patients, audit, dir, [this] { return now; }};   // saved to disk
    ASSERT_TRUE(reloaded.load().ok());
    auto h = reloaded.history(doc1, "P000001").value();
    const Consultation& first = h[2];
    EXPECT_EQ(first.diagnosis, "Visit 1");                                    // original unchanged
    ASSERT_EQ(first.addenda.size(), 1u);
    EXPECT_EQ(first.addenda[0].text, "Dose changed to 5 mg");
    EXPECT_EQ(first.addenda[0].timestamp, "2026-11-10 10:00");
    EXPECT_EQ(first.addenda[0].authorUserId, "U90001");
}
