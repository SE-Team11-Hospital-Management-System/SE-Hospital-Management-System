// Member 1 (SF-1) - central permission check (HMS-SEC-002 part): role first (E004 + audit), then session (E003)
#include "security/access_control.h"
#include "security/audit_service.h"
#include "test_helpers.h"
#include <filesystem>
#include <gtest/gtest.h>

using namespace hms;

TEST(AccessCheck, DeniedRoleIsE004AndAuditLogged) {
    const std::string dir = hms_test::freshDir("acc");
    AuditService audit(dir + "/audit.log");
    SessionManager sessions;
    AccessControl::attach(&sessions, &audit);
    Session rec = sessions.create("U00002", Role::Receptionist);
    auto r = AccessControl::authorize(rec, Operation::Backup, "backup");
    ASSERT_FALSE(r.ok());
    EXPECT_EQ(r.error().code, ErrorCode::E004);
    auto log = audit.readAll();
    ASSERT_EQ(log.size(), 1u);
    EXPECT_EQ(log[0].action, "ACCESS_DENIED");
    EXPECT_EQ(log[0].targetId, "backup");
    EXPECT_EQ(log[0].userId, "U00002");
    EXPECT_TRUE(AccessControl::authorize(rec, Operation::RegisterPatient, "registerPatient").ok());
    AccessControl::detach(&sessions);
    std::filesystem::remove_all(dir);
}

TEST(AccessCheck, UnknownOrEndedSessionIsE003) {
    SessionManager sessions;
    AccessControl::attach(&sessions, nullptr);
    Session admin = sessions.create("U00001", Role::Admin);
    EXPECT_TRUE(AccessControl::authorize(admin, Operation::Backup, "backup").ok());
    sessions.end(admin.token);
    EXPECT_EQ(AccessControl::authorize(admin, Operation::Backup, "backup").error().code, ErrorCode::E003);
    EXPECT_EQ(AccessControl::authorize(hms_test::session(Role::Admin), Operation::Backup, "backup").error().code, ErrorCode::E003);
    AccessControl::detach(&sessions);
    EXPECT_TRUE(AccessControl::authorize(hms_test::session(Role::Admin), Operation::Backup, "backup").ok());   // not attached: role only
}

TEST(AccessCheck, AppContextAttachesItself) {
    hms_test::World w;
    EXPECT_EQ(w.ctx->doctors.addDoctor(w.rec, {"X Y", "ENT", "ENT", "U00003"}).error().code, ErrorCode::E004);
    bool logged = false;
    for (const auto& e : w.ctx->audit.readAll()) logged |= (e.action == "ACCESS_DENIED" && e.targetId == "addDoctor");
    EXPECT_TRUE(logged);
    EXPECT_TRUE(w.ctx->doctors.findByUser("U00003").has_value());
    EXPECT_EQ(w.ctx->doctors.findByUser("U00003")->doctorId, w.d1);
}
