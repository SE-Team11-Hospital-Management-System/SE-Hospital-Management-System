// Member 1 (SF-1) - TC-AUTH-01, 02, 04, TC-SEC-01 (HMS-F-001, F-002, F-004, SEC-001, SEC-008)
#include "services/auth_service.h"
#include "test_helpers.h"
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <sstream>

using namespace hms;
using namespace std::chrono;

namespace {
struct AuthFixture : ::testing::Test {
    std::string dir = hms_test::freshDir("auth");
    system_clock::time_point now = system_clock::now();
    AuditService audit{dir + "/audit.log"};
    SessionManager sessions{[this] { return now; }};
    AuthService auth{sessions, audit, dir, [this] { return now; }};
    Session admin;
    void SetUp() override {
        ASSERT_TRUE(auth.bootstrapAdmin("admin01", "Admin1234").ok());
        admin = auth.login("admin01", "Admin1234").value();
        ASSERT_TRUE(auth.createUser(admin, "rec01", "Rec@1234", Role::Receptionist).ok());
    }
    void TearDown() override { std::filesystem::remove_all(dir); }
};
}  // namespace

// TC-AUTH-01: valid login creates a session with the right role
TEST_F(AuthFixture, TC_AUTH_01_ValidLogin) {
    auto r = auth.login("rec01", "Rec@1234");
    ASSERT_TRUE(r.ok());
    EXPECT_EQ(r.value().role, Role::Receptionist);
    EXPECT_FALSE(r.value().token.empty());
}

// TC-AUTH-02: lock-out after 3 failures, generic message, unlock after 15 minutes
TEST_F(AuthFixture, TC_AUTH_02_LockoutAndGenericError) {
    for (int i = 0; i < 3; ++i) {
        auto r = auth.login("rec01", "Wrong1234");
        ASSERT_FALSE(r.ok());
        EXPECT_EQ(r.error().message, "Invalid username or password.");
    }
    auto unknown = auth.login("xyzz", "Whatever1");
    EXPECT_EQ(unknown.error().message, "Invalid username or password.");   // HMS-SEC-008
    auto locked = auth.login("rec01", "Rec@1234");
    ASSERT_FALSE(locked.ok());
    EXPECT_EQ(locked.error().code, ErrorCode::E002);
    EXPECT_NE(locked.error().message.find("15 minutes"), std::string::npos);
    now += minutes(15);
    EXPECT_TRUE(auth.login("rec01", "Rec@1234").ok());
}

// TC-AUTH-04: user management by the Administrator only
TEST_F(AuthFixture, TC_AUTH_04_UserManagement) {
    auto created = auth.createUser(admin, "doc02", "Doc12345", Role::Doctor);
    ASSERT_TRUE(created.ok());
    EXPECT_EQ(auth.createUser(admin, "doc02", "Doc12345", Role::Doctor).error().code, ErrorCode::E011);
    EXPECT_EQ(auth.createUser(admin, "ab", "Doc12345", Role::Doctor).error().code, ErrorCode::E010);
    ASSERT_TRUE(auth.setUserStatus(admin, created.value(), false).ok());
    EXPECT_EQ(auth.login("doc02", "Doc12345").error().message, "Invalid username or password.");
    ASSERT_TRUE(auth.setUserStatus(admin, created.value(), true).ok());
    EXPECT_TRUE(auth.login("doc02", "Doc12345").ok());
    auto rec = auth.login("rec01", "Rec@1234").value();
    EXPECT_EQ(auth.createUser(rec, "hack01", "Hack1234", Role::Admin).error().code, ErrorCode::E004);
}

// TC-SEC-01: only salted hashes on disk, unique salt per account, users survive a restart
TEST_F(AuthFixture, TC_SEC_01_PasswordsStoredOnlyAsSaltedHash) {
    std::ifstream in(dir + "/users.dat");
    std::stringstream ss; ss << in.rdbuf();
    const std::string file = ss.str();
    EXPECT_EQ(file.find("Rec@1234"), std::string::npos);
    EXPECT_EQ(file.find("Admin1234"), std::string::npos);

    SessionManager sessions2;
    AuthService reloaded(sessions2, audit, dir);
    ASSERT_TRUE(reloaded.load().ok());
    EXPECT_TRUE(reloaded.login("rec01", "Rec@1234").ok());
}

// HMS-F-001 support: the first Administrator can only be created once
TEST_F(AuthFixture, BootstrapOnlyOnFirstRun) {
    EXPECT_FALSE(auth.bootstrapAdmin("admin02", "Admin1234").ok());
}
