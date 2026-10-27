// Member 1 (SF-1) - TC-SEC-04 idle time-out, TC-AUTH-05 change password, TC-SEC-03 audit filter
// (HMS-F-005, HMS-F-006, HMS-F-062)
#include "common/datetime.h"
#include "security/access_control.h"
#include "services/auth_service.h"
#include "test_helpers.h"
#include <filesystem>
#include <gtest/gtest.h>

using namespace hms;
using namespace std::chrono;

// TC-SEC-04: an action after 9 min 59 s works and resets the timer; after 10 min 1 s it is refused (E003)
TEST(Session, TC_SEC_04_IdleTimeout) {
    system_clock::time_point now = system_clock::now();
    SessionManager sessions([&] { return now; });
    AccessControl::attach(&sessions, nullptr);
    Session rec = sessions.create("U00002", Role::Receptionist);
    now += minutes(9) + seconds(59);
    EXPECT_TRUE(AccessControl::authorize(rec, Operation::RegisterPatient, "registerPatient").ok());
    now += minutes(9) + seconds(59);                                          // timer was reset by the last action
    EXPECT_TRUE(AccessControl::authorize(rec, Operation::RegisterPatient, "registerPatient").ok());
    now += minutes(10) + seconds(1);
    auto r = AccessControl::authorize(rec, Operation::RegisterPatient, "registerPatient");
    ASSERT_FALSE(r.ok());
    EXPECT_EQ(r.error().code, ErrorCode::E003);
    EXPECT_EQ(sessions.validate(rec.token).error().code, ErrorCode::E003);    // a new login is required
    AccessControl::detach(&sessions);
}

// TC-AUTH-05: wrong current password, too short, no digit, same as current are refused; a valid change works
TEST(Password, TC_AUTH_05_ChangePassword) {
    hms_test::World w;
    auto& auth = w.ctx->auth;
    EXPECT_EQ(auth.changePassword(w.rec, "Wrong1234", "NewPass99").error().code, ErrorCode::E001);
    auto shortPwd = auth.changePassword(w.rec, "Rec@1234", "abc123");
    EXPECT_EQ(shortPwd.error().message, "New password: must have 8-32 characters.");
    auto noDigit = auth.changePassword(w.rec, "Rec@1234", "abcdefgh");
    EXPECT_EQ(noDigit.error().message, "New password: must contain at least one letter and one digit.");
    EXPECT_EQ(auth.changePassword(w.rec, "Rec@1234", "Rec@1234").error().message, "New password: must differ from the current password.");
    ASSERT_TRUE(auth.changePassword(w.rec, "Rec@1234", "NewPass99").ok());
    EXPECT_EQ(auth.login("rec01", "Rec@1234").error().code, ErrorCode::E001);
    EXPECT_TRUE(auth.login("rec01", "NewPass99").ok());

    SessionManager s2;                                                        // the new password survives a restart
    AuthService again(s2, w.ctx->audit, w.dir);
    ASSERT_TRUE(again.load().ok());
    EXPECT_TRUE(again.login("rec01", "NewPass99").ok());
}

// TC-SEC-03 (viewer part): filter by date range and by user; Receptionist cannot open the log
TEST(AuditViewer, TC_SEC_03_FilterByDateAndUser) {
    const std::string dir = hms_test::freshDir("auditq");
    AuditService audit(dir + "/audit.log");
    audit.log("U00001", "LOGIN_OK", "U00001", true);
    audit.log("U00002", "LOGIN_OK", "U00002", true);
    audit.log("U00002", "PATIENT_CREATE", "P000001", true);
    const std::string today = dt::today();
    EXPECT_EQ(audit.query(today, today).size(), 3u);
    EXPECT_EQ(audit.query(today, today, "U00002").size(), 2u);
    EXPECT_TRUE(audit.query("2000-01-01", "2000-12-31").empty());
    EXPECT_FALSE(AccessControl::isAllowed(Role::Receptionist, Operation::ViewAuditLog));
    std::filesystem::remove_all(dir);
}
