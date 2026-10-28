// Member 2 (SF-2) - TC-SEC-06 through the website: damaged data -> recovery mode -> Administrator restores (HMS-SEC-007)
#include "api_helpers.h"
#include "data/file_store.h"
#include <fstream>
#include <gtest/gtest.h>

using namespace hms;
using hms_test::ApiWorld;

TEST(WebApi, TC_SEC_06_RecoveryModeAndRestore) {
    ApiWorld a;
    ASSERT_TRUE(a.w.ctx->backup.createBackup(a.w.admin).ok());
    {
        std::fstream f(a.w.dir + "/patients.dat", std::ios::in | std::ios::out | std::ios::binary);
        f.seekp(3); f.put('#');
    }
    auto loaded = loadAllData(*a.w.ctx);                                          // what main() does at start-up
    ASSERT_EQ(loaded.error().code, ErrorCode::E040);
    a.api.enterRecovery(loaded.error());

    auto status = a.get("/api/status");
    EXPECT_TRUE(status.body["recovery"]["needed"].get<bool>());
    EXPECT_EQ(status.body["recovery"]["file"], "patients.dat");
    EXPECT_EQ(a.post("/api/login", {{"username", "admin01"}, {"password", "Admin1234"}}).code(), "E040");   // blocked
    EXPECT_EQ(a.post("/api/recovery/restore", {{"username", "rec01"}, {"password", "Rec@1234"}}).code(), "E004");
    auto ok = a.post("/api/recovery/restore", {{"username", "admin01"}, {"password", "Admin1234"}});
    ASSERT_EQ(ok.status, 200) << ok.raw;
    EXPECT_FALSE(a.get("/api/status").body["recovery"]["needed"].get<bool>());
    EXPECT_FALSE(a.login("admin01", "Admin1234").empty());
    bool logged = false;
    for (const auto& e : a.w.ctx->audit.readAll()) logged |= (e.action == "INTEGRITY_FAIL" && e.targetId == "patients.dat");
    EXPECT_TRUE(logged);
}
