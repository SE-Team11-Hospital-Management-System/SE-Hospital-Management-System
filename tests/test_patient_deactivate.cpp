// Member 2 (SF-2) - TC-PAT-06 (HMS-F-014)
#include "test_helpers.h"
#include <fstream>
#include <gtest/gtest.h>
#include <sstream>

using namespace hms;

TEST(PatientDeactivate, TC_PAT_06_AdminOnlyNeverDeleted) {
    hms_test::World w;
    EXPECT_EQ(w.ctx->patients.deactivatePatient(w.rec, w.p1).error().code, ErrorCode::E004);
    ASSERT_TRUE(w.ctx->patients.deactivatePatient(w.admin, w.p1).ok());
    EXPECT_EQ(w.ctx->patients.deactivatePatient(w.admin, w.p1).error().code, ErrorCode::E032);   // already inactive
    EXPECT_EQ(w.ctx->patients.deactivatePatient(w.admin, "P999999").error().code, ErrorCode::E020);

    auto found = w.ctx->patients.search(w.rec, SearchKey::Id, w.p1).value().items;
    ASSERT_EQ(found.size(), 1u);
    EXPECT_FALSE(found[0].active);                                         // search still shows it (labelled Inactive)
    EXPECT_EQ(w.ctx->appointments.book(w.rec, w.p1, w.d1, "2030-01-07 09:00").error().code, ErrorCode::E025);

    std::ifstream in(w.dir + "/patients.dat");
    std::stringstream ss; ss << in.rdbuf();
    EXPECT_NE(ss.str().find("Anita Rao"), std::string::npos);              // still stored, not deleted
    bool audited = false;
    for (const auto& e : w.ctx->audit.readAll()) audited |= (e.action == "PATIENT_DEACTIVATE" && e.targetId == w.p1);
    EXPECT_TRUE(audited);

    PatientRepository repo2;                                               // survives a restart as Inactive
    ASSERT_TRUE(repo2.load(w.dir + "/patients.dat").ok());
    EXPECT_FALSE(repo2.findById(w.p1)->active);
}
