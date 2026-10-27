// Member 2 (SF-2) - web foundation (HMS-F-070): status, first run, login through the website, sessions, website files
#include "api_helpers.h"
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

using namespace hms;
using hms_test::ApiWorld;

TEST(WebApi, StatusLoginAndSession) {
    ApiWorld a;
    auto status = a.get("/api/status");
    EXPECT_EQ(status.status, 200);
    EXPECT_TRUE(status.body["hasUsers"].get<bool>());
    EXPECT_FALSE(status.body["recovery"]["needed"].get<bool>());

    auto wrong = a.post("/api/login", {{"username", "rec01"}, {"password", "Wrong9999"}});
    EXPECT_EQ(wrong.status, 401);
    EXPECT_EQ(wrong.body["error"]["message"], "Invalid username or password.");   // HMS-SEC-008

    const std::string token = a.login("rec01", "Rec@1234");
    ASSERT_FALSE(token.empty());
    auto me = a.get("/api/me", token);
    ASSERT_EQ(me.status, 200);
    EXPECT_EQ(me.body["role"], "Receptionist");
    auto perms = me.body["permissions"];
    EXPECT_NE(std::find(perms.begin(), perms.end(), "registerPatient"), perms.end());
    EXPECT_EQ(std::find(perms.begin(), perms.end(), "viewMedicalHistory"), perms.end());

    EXPECT_EQ(a.get("/api/me").code(), "E003");                                // no token
    EXPECT_EQ(a.get("/api/me", "not-a-token").status, 401);
    EXPECT_EQ(a.post("/api/logout", json::object(), token).status, 200);
    EXPECT_EQ(a.get("/api/me", token).code(), "E003");                         // logged out
}

TEST(WebApi, DoctorLoginCarriesOwnProfile) {
    ApiWorld a;
    auto r = a.post("/api/login", {{"username", "doc01"}, {"password", "Doc12345"}});
    ASSERT_EQ(r.status, 200);
    EXPECT_EQ(r.body["user"]["role"], "Doctor");
    EXPECT_EQ(r.body["user"]["doctorId"], a.w.d1);
}

TEST(WebApi, FirstRunSetupOnlyOnce) {
    ApiWorld a;
    EXPECT_EQ(a.post("/api/setup", {{"username", "admin02"}, {"password", "Admin1234"}}).code(), "E004");
}

TEST(WebApi, BadJsonIsE010) {
    ApiWorld a;
    auto r = ApiWorld::wrap(a.http->Post("/api/login", "{not json", "application/json"));
    EXPECT_EQ(r.status, 400);
    EXPECT_EQ(r.code(), "E010");
}

TEST(WebApi, ServesTheWebsiteAndFallsBackToIndex) {
    const std::string web = hms_test::freshDir("web");
    std::ofstream(web + "/index.html") << "<html>HMS</html>";
    std::filesystem::create_directories(web + "/assets");
    std::ofstream(web + "/assets/app.js") << "console.log(1)";
    ApiWorld a(web);
    auto page = a.http->Get("/patients");                                         // a React page address
    ASSERT_TRUE(page);
    EXPECT_EQ(page->status, 200);
    EXPECT_NE(page->body.find("HMS"), std::string::npos);
    auto js = a.http->Get("/assets/app.js");
    ASSERT_TRUE(js);
    EXPECT_EQ(js->body, "console.log(1)");
    std::filesystem::remove_all(web);
}

TEST(WebApi, RefusesRequestsFromOtherSites) {                  // HMS-SEC-009: DNS rebinding and cross-site requests
    ApiWorld a;
    auto evilHost = ApiWorld::wrap(a.http->Get("/api/status", {{"Host", "evil.example:8080"}}));
    EXPECT_EQ(evilHost.status, 403);
    auto evilOrigin = ApiWorld::wrap(a.http->Post("/api/login", {{"Origin", "http://evil.example"}},
                                                  json{{"username", "rec01"}, {"password", "Rec@1234"}}.dump(), "application/json"));
    EXPECT_EQ(evilOrigin.status, 403);
    auto local = ApiWorld::wrap(a.http->Post("/api/login", {{"Origin", "http://localhost:8080"}},
                                             json{{"username", "rec01"}, {"password", "Rec@1234"}}.dump(), "application/json"));
    EXPECT_EQ(local.status, 200);
    EXPECT_EQ(ApiWorld::wrap(a.http->Get("/api/status", {{"Host", "localhost:8080"}})).status, 200);
}
