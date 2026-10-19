// Member 1 (SF-1) - security core: role matrix (HMS-F-003 / SRS Table 6.5-1), salted hashing (HMS-SEC-001), sessions
#include "security/access_control.h"
#include "security/audit_service.h"
#include "security/password_hasher.h"
#include "security/session_manager.h"
#include "test_helpers.h"
#include <filesystem>
#include <gtest/gtest.h>

using namespace hms;

// TC-AUTH-03 (Sprint-1 part): role matrix = SRS Table 6.5-1, deny by default
TEST(SecurityCore, TC_AUTH_03_RoleMatrix) {
    EXPECT_TRUE(AccessControl::isAllowed(Role::Admin, Operation::ManageUsers));
    EXPECT_FALSE(AccessControl::isAllowed(Role::Receptionist, Operation::ManageUsers));
    EXPECT_FALSE(AccessControl::isAllowed(Role::Doctor, Operation::RegisterPatient));
    EXPECT_TRUE(AccessControl::isAllowed(Role::Doctor, Operation::SearchPatient));
    EXPECT_FALSE(AccessControl::isAllowed(Role::Receptionist, Operation::Backup));
    EXPECT_FALSE(AccessControl::isAllowed(Role::Admin, Operation::RecordConsultation));
    EXPECT_TRUE(AccessControl::isAllowed(Role::Receptionist, Operation::BookAppointment));
    EXPECT_FALSE(AccessControl::isAllowed(Role::Doctor, Operation::BookAppointment));
}

// TC-SEC-01 (part): unique 16-byte salt per account, same password -> different hash, verify works
TEST(SecurityCore, TC_SEC_01_SaltedHash) {
    const std::string s1 = PasswordHasher::generateSalt(), s2 = PasswordHasher::generateSalt();
    EXPECT_EQ(s1.size(), 32u);                      // 16 bytes as hex
    EXPECT_NE(s1, s2);
    const std::string h1 = PasswordHasher::hash("Same1234", s1);
    EXPECT_NE(h1, PasswordHasher::hash("Same1234", s2));
    EXPECT_EQ(h1.find("Same1234"), std::string::npos);
    EXPECT_TRUE(PasswordHasher::verify("Same1234", s1, h1));
    EXPECT_FALSE(PasswordHasher::verify("Same1235", s1, h1));
}

TEST(SecurityCore, SessionsAreUniqueAndEnd) {
    SessionManager sm;
    auto a = sm.create("U00001", Role::Admin), b = sm.create("U00001", Role::Admin);
    EXPECT_NE(a.token, b.token);
    EXPECT_TRUE(sm.validate(a.token).ok());
    sm.end(a.token);
    EXPECT_EQ(sm.validate(a.token).error().code, ErrorCode::E003);
}

TEST(SecurityCore, AuditIsAppendOnlyAndHasNoSecrets) {
    const std::string dir = hms_test::freshDir("audit");
    AuditService audit(dir + "/audit.log");
    audit.log("U00001", "LOGIN_OK", "U00001", true);
    audit.log("", "LOGIN_FAIL", "", false);
    auto all = audit.readAll();
    ASSERT_EQ(all.size(), 2u);
    EXPECT_EQ(all[0].action, "LOGIN_OK");
    EXPECT_EQ(all[1].userId, "-");
    EXPECT_EQ(all[1].outcome, "FAILURE");
    std::filesystem::remove_all(dir);
}
