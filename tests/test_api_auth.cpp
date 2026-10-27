// Member 1 (SF-1) - web routes for password, users and audit log (HMS-F-004, F-006, F-062, SEC-002)
#include "api_helpers.h"
#include "common/datetime.h"
#include <gtest/gtest.h>

using namespace hms;
using hms_test::ApiWorld;

TEST(WebApi, PasswordUsersAndAudit) {
    ApiWorld a;
    const std::string admin = a.login("admin01", "Admin1234"), rec = a.login("rec01", "Rec@1234");
    auto shortPwd = a.post("/api/password", {{"current", "Rec@1234"}, {"new", "abc123"}}, rec);
    EXPECT_EQ(shortPwd.status, 400);
    EXPECT_EQ(shortPwd.body["error"]["message"], "New password: must have 8-32 characters.");
    EXPECT_EQ(a.post("/api/password", {{"current", "Rec@1234"}, {"new", "NewPass99"}}, rec).status, 200);
    EXPECT_FALSE(a.login("rec01", "NewPass99").empty());

    EXPECT_EQ(a.get("/api/users", rec).status, 403);                                      // Receptionist
    auto users = a.get("/api/users", admin);
    ASSERT_EQ(users.status, 200);
    EXPECT_EQ(users.body.size(), 4u);
    auto created = a.post("/api/users", {{"username", "rec02"}, {"password", "Rec@5678"}, {"role", "Receptionist"}}, admin);
    ASSERT_EQ(created.status, 201);
    const std::string id = created.body["userId"];
    EXPECT_EQ(a.post("/api/users", {{"username", "rec02"}, {"password", "Rec@5678"}, {"role", "Receptionist"}}, admin).code(), "E011");
    EXPECT_EQ(a.post("/api/users/" + id + "/status", {{"active", false}}, admin).status, 200);
    EXPECT_TRUE(a.login("rec02", "Rec@5678").empty());

    const std::string today = dt::today();
    auto audit = a.get("/api/audit?from=" + today + "&to=" + today + "&username=rec01", admin);
    ASSERT_EQ(audit.status, 200);
    for (const auto& e : audit.body) EXPECT_EQ(e["userId"], "U00002");
    EXPECT_GT(audit.body.size(), 0u);
    EXPECT_EQ(a.get("/api/audit", rec).status, 403);
    EXPECT_EQ(a.get("/api/audit?from=2026-11-01&to=2026-10-01", admin).code(), "E010");
    EXPECT_EQ(a.get("/api/audit?username=nobody", admin).code(), "E023");
}
