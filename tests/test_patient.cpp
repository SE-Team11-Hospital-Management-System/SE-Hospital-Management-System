// Member 2 (SF-2) - TC-PAT-01..05, TC-SEC-02 (HMS-F-010..013, F-015, SEC-005)
#include "services/patient_service.h"
#include "test_helpers.h"
#include <filesystem>
#include <gtest/gtest.h>

using namespace hms;
using hms_test::session;

namespace {
struct PatientFixture : ::testing::Test {
    std::string dir = hms_test::freshDir("pat");
    AuditService audit{dir + "/audit.log"};
    PatientRepository repo;
    PatientService svc{repo, audit, dir, [] { return std::string("2026-10-19"); }};
    Session rec = session(Role::Receptionist, "U00002");
    PatientInput anita{"Anita Rao", "1990-05-14", "F", "9876543210", "12 MG Road, Bengaluru", "B+"};
    void TearDown() override { std::filesystem::remove_all(dir); }
};
}  // namespace

// TC-PAT-01: register, IDs P000001 / P000002, data present after restart
TEST_F(PatientFixture, TC_PAT_01_RegisterAndReload) {
    auto a = svc.registerPatient(rec, anita);
    ASSERT_TRUE(a.ok());
    EXPECT_EQ(a.value(), "P000001");
    auto b = svc.registerPatient(rec, {"Anil Kumar", "1985-01-01", "M", "9123456780", "Jayanagar", "O+"});
    EXPECT_EQ(b.value(), "P000002");

    PatientRepository repo2;
    PatientService svc2{repo2, audit, dir, [] { return std::string("2026-10-19"); }};
    ASSERT_TRUE(svc2.load().ok());
    auto p = svc2.get("P000001");
    ASSERT_TRUE(p.has_value());
    EXPECT_EQ(p->address, "12 MG Road, Bengaluru");
    EXPECT_EQ(repo2.nextId(), "P000003");
}

// TC-PAT-02: each invalid field is rejected with a field-specific E010 and nothing is saved
TEST_F(PatientFixture, TC_PAT_02_Validation) {
    auto bad = [&](PatientInput in, const std::string& field) {
        auto r = svc.registerPatient(rec, in);
        ASSERT_FALSE(r.ok());
        EXPECT_EQ(r.error().code, ErrorCode::E010);
        EXPECT_EQ(r.error().message.rfind(field, 0), 0u) << r.error().message;
    };
    PatientInput in = anita; in.fullName = "";              bad(in, "Full name");
    in = anita; in.dob = "2030-01-01";                      bad(in, "Date of birth");
    in = anita; in.phone = "98765";                         bad(in, "Phone");
    in = anita; in.phone = "98765abcde";                    bad(in, "Phone");
    in = anita; in.fullName = std::string(61, 'a');         bad(in, "Full name");
    EXPECT_EQ(repo.size(), 0u);
}

// TC-PAT-03: duplicate warning shows the existing ID; confirming creates a new record
TEST_F(PatientFixture, TC_PAT_03_DuplicateNeedsConfirmation) {
    ASSERT_TRUE(svc.registerPatient(rec, anita).ok());
    auto dup = svc.registerPatient(rec, anita);
    ASSERT_FALSE(dup.ok());
    EXPECT_EQ(dup.error().code, ErrorCode::E011);
    EXPECT_EQ(dup.error().details, "P000001");
    EXPECT_EQ(repo.size(), 1u);
    auto forced = svc.registerPatient(rec, anita, true);
    ASSERT_TRUE(forced.ok());
    EXPECT_EQ(forced.value(), "P000002");
}

// TC-PAT-04: search by ID, partial name (case-insensitive), phone, paging 20 per page
TEST_F(PatientFixture, TC_PAT_04_SearchAndPaging) {
    ASSERT_TRUE(svc.registerPatient(rec, anita).ok());
    ASSERT_TRUE(svc.registerPatient(rec, {"Anil Kumar", "1985-01-01", "M", "9123456780", "Jayanagar", "O+"}).ok());
    for (int i = 0; i < 23; ++i)
        ASSERT_TRUE(svc.registerPatient(rec, {"Rao Test", "2000-01-01", "M", "90000000" + std::to_string(10 + i), "X", "A+"}).ok());
    EXPECT_EQ(svc.search(rec, SearchKey::Id, "P000001").value().items.size(), 1u);
    EXPECT_EQ(svc.search(rec, SearchKey::Name, "ANI").value().items.size(), 2u);
    EXPECT_EQ(svc.search(rec, SearchKey::Phone, "9876543210").value().items.size(), 1u);
    EXPECT_TRUE(svc.search(rec, SearchKey::Name, "zzzz").value().items.empty());
    auto page1 = svc.search(rec, SearchKey::Name, "rao", 1).value();
    EXPECT_EQ(page1.items.size(), 20u);
    EXPECT_TRUE(page1.hasNext);
    auto page2 = svc.search(rec, SearchKey::Name, "rao", 2).value();
    EXPECT_EQ(page2.items.size(), 4u);
    EXPECT_FALSE(page2.hasNext);
}

// TC-PAT-05: update phone + address; ID and DOB never change; audit holds field names only
TEST_F(PatientFixture, TC_PAT_05_UpdateWithAudit) {
    ASSERT_TRUE(svc.registerPatient(rec, anita).ok());
    ASSERT_TRUE(svc.updatePatient(rec, "P000001", {"", "2001-01-01", "M", "9000000001", "New Address", ""}).ok());
    auto p = svc.get("P000001").value();
    EXPECT_EQ(p.phone, "9000000001");
    EXPECT_EQ(p.address, "New Address");
    EXPECT_EQ(p.dob, "1990-05-14");
    EXPECT_EQ(p.gender, "F");
    bool found = false;
    for (const auto& e : audit.readAll())
        if (e.action == "PATIENT_UPDATE") { found = true; EXPECT_EQ(e.targetId, "P000001:phone,address"); EXPECT_EQ(e.userId, "U00002"); }
    EXPECT_TRUE(found);
    EXPECT_EQ(svc.updatePatient(rec, "P999999", {}).error().code, ErrorCode::E020);
}

// TC-SEC-02 + role check: 5,000-character input is rejected without crashing; a Doctor cannot register
TEST_F(PatientFixture, TC_SEC_02_LongInputAndRole) {
    PatientInput in = anita; in.address = std::string(5000, 'x');
    EXPECT_EQ(svc.registerPatient(rec, in).error().code, ErrorCode::E010);
    EXPECT_EQ(svc.search(rec, SearchKey::Name, std::string(5000, 'a')).error().code, ErrorCode::E010);
    EXPECT_EQ(svc.registerPatient(session(Role::Doctor), anita).error().code, ErrorCode::E004);
    EXPECT_EQ(repo.size(), 0u);
}
