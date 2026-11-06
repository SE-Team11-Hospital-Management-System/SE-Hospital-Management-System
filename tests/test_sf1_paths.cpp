// Member 1 (SF-1) - Part-3: the error paths of login, user accounts, passwords and the audit log that the Sprint
// tests did not reach (TC-AUTH-01, TC-AUTH-04, TC-AUTH-05, TC-SEC-03, TC-WEB-01, TC-WEB-02, TC-DATA-01).
#include "api_helpers.h"
#include "data/file_store.h"
#include <filesystem>
#include <fstream>
#include <set>
#include <gtest/gtest.h>

using namespace hms;
using hms_test::ApiWorld;
namespace fs = std::filesystem;

namespace {
// A server with no accounts yet, like the very first start of HMS.
struct FreshApi {
    std::string root = hms_test::freshDir("fresh");
    AppContext ctx{root + "/data", root + "/backup", root + "/reports"};
    Api api{ctx, ""};
    std::thread thread;
    std::unique_ptr<httplib::Client> http;
    FreshApi() {
        (void)loadAllData(ctx);
        registerAllRoutes(api);
        const int port = api.bind(Api::kHost, 0);
        thread = std::thread([this] { api.serve(); });
        http = std::make_unique<httplib::Client>(Api::kHost, port);
        for (int i = 0; i < 100 && !http->Get("/api/status"); ++i) std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    ~FreshApi() { api.stop(); if (thread.joinable()) thread.join(); std::error_code ec; fs::remove_all(root, ec); }
    ApiWorld::Reply post(const std::string& path, const json& body) { return ApiWorld::wrap(http->Post(path, body.dump(), "application/json")); }
};
}  // namespace

// TC-AUTH-01: first-run rules and the input checks done before any password is compared
TEST(SF1Paths, TC_AUTH_01_FirstRunAndLoginInputRules) {
    const std::string dir = hms_test::freshDir("sf1login");
    SessionManager sessions;
    AuditService audit(dir + "/audit.log");
    AuthService auth(sessions, audit, dir);
    EXPECT_FALSE(auth.hasUsers());
    EXPECT_EQ(auth.login("admin01", "Admin1234").error().code, ErrorCode::E001);          // no accounts yet
    EXPECT_EQ(auth.bootstrapAdmin("ab", "Admin1234").error().code, ErrorCode::E010);       // username too short
    EXPECT_EQ(auth.bootstrapAdmin("admin01", "short1").error().code, ErrorCode::E010);     // password too short
    EXPECT_EQ(auth.bootstrapAdmin("admin01", "lettersonly").error().code, ErrorCode::E010);
    ASSERT_TRUE(auth.bootstrapAdmin("admin01", "Admin1234").ok());
    EXPECT_TRUE(auth.hasUsers());
    EXPECT_EQ(auth.login("admin01", std::string(300, 'a')).error().message, "Invalid username or password.");   // HMS-SEC-008
    EXPECT_EQ(auth.login("admin01", std::string("Admin\x01" "1234")).error().code, ErrorCode::E001);
    EXPECT_EQ(auth.login("bad name!", "Admin1234").error().code, ErrorCode::E001);
    EXPECT_FALSE(auth.findUser("U99999").has_value());
    size_t fails = 0;
    for (const auto& e : audit.readAll()) fails += e.action == "LOGIN_FAIL";
    EXPECT_EQ(fails, 4u);
    fs::remove_all(dir);
}

// TC-AUTH-04: account status rules - unknown user, own account, deactivated user cannot log in
TEST(SF1Paths, TC_AUTH_04_UserStatusRules) {
    hms_test::World w;
    auto& auth = w.ctx->auth;
    EXPECT_EQ(auth.setUserStatus(w.admin, "U99999", false).error().code, ErrorCode::E023);
    EXPECT_EQ(auth.setUserStatus(w.admin, w.admin.userId, false).error().code, ErrorCode::E010);   // never lock yourself out
    EXPECT_EQ(auth.setUserStatus(w.rec, "U00003", false).error().code, ErrorCode::E004);
    EXPECT_EQ(auth.listUsers(w.rec).error().code, ErrorCode::E004);
    EXPECT_EQ(auth.createUser(w.admin, "rec09", "nodigits", Role::Receptionist).error().code, ErrorCode::E010);
    ASSERT_TRUE(auth.setUserStatus(w.admin, "U00002", false).ok());
    EXPECT_EQ(auth.login("rec01", "Rec@1234").error().message, "Invalid username or password.");
    ASSERT_TRUE(auth.setUserStatus(w.admin, "U00002", true).ok());
    EXPECT_TRUE(auth.login("rec01", "Rec@1234").ok());
}

// TC-DATA-01 for users.dat: a failed save changes nothing (E041) - new user, status, password
TEST(SF1Paths, TC_DATA_01_FailedSaveOfUsersChangesNothing) {
    hms_test::World w;
    auto& auth = w.ctx->auth;
    fs::create_directory(w.dir + "/users.dat.tmp");                                         // the next save fails
    EXPECT_EQ(auth.createUser(w.admin, "rec05", "Rec@5678", Role::Receptionist).error().code, ErrorCode::E041);
    EXPECT_EQ(auth.setUserStatus(w.admin, "U00002", false).error().code, ErrorCode::E041);
    EXPECT_EQ(auth.changePassword(w.rec, "Rec@1234", "NewPass99").error().code, ErrorCode::E041);
    fs::remove(w.dir + "/users.dat.tmp");
    EXPECT_EQ(auth.listUsers(w.admin).value().size(), 4u);
    EXPECT_TRUE(auth.findUser("U00002")->active);
    EXPECT_TRUE(auth.login("rec01", "Rec@1234").ok());                                        // old password still works

    const std::string dir = hms_test::freshDir("sf1boot");
    SessionManager s;
    AuditService a(dir + "/audit.log");
    AuthService first(s, a, dir);
    fs::create_directory(dir + "/users.dat.tmp");
    EXPECT_EQ(first.bootstrapAdmin("admin01", "Admin1234").error().code, ErrorCode::E041);
    EXPECT_FALSE(first.hasUsers());
    fs::remove_all(dir);
}

// TC-SEC-06 for users.dat: a record with the wrong number of fields is refused even with a correct checksum
TEST(SF1Paths, TC_SEC_06_MalformedUsersFileIsE040) {
    const std::string dir = hms_test::freshDir("sf1bad");
    ASSERT_TRUE(FileStore::atomicWrite(dir + "/users.dat", {"U00001|admin01|0"}).ok());
    SessionManager s;
    AuditService a(dir + "/audit.log");
    AuthService auth(s, a, dir);
    EXPECT_EQ(auth.load().error().code, ErrorCode::E040);
    fs::remove_all(dir);
}

// TC-SEC-03: the audit viewer skips damaged lines; a log that cannot be opened never stops the program
TEST(SF1Paths, TC_SEC_03_AuditToleratesDamagedLinesAndMissingFolder) {
    const std::string dir = hms_test::freshDir("sf1audit");
    { std::ofstream(dir + "/audit.log") << "not|enough|fields\n" << "2030-01-07T09:00:00|U00001|LOGIN_OK|U00001|SUCCESS\n"; }
    AuditService audit(dir + "/audit.log");
    ASSERT_EQ(audit.readAll().size(), 1u);
    EXPECT_EQ(audit.query("2030-01-07", "2030-01-07", "U00002").size(), 0u);
    EXPECT_EQ(audit.query("2030-01-07", "2030-01-07", "U00001").size(), 1u);
    AuditService nowhere(dir + "/no-such-folder/audit.log");
    nowhere.log("U00001", "LOGIN_OK", "U00001", true);                                       // must not crash
    EXPECT_TRUE(nowhere.readAll().empty());
    fs::remove_all(dir);
}

// TC-WEB-01: first run through the website - only one first Administrator, and its input rules
TEST(SF1Paths, TC_WEB_01_FirstRunSetupThroughTheWebsite) {
    FreshApi f;
    auto st = ApiWorld::wrap(f.http->Get("/api/status"));
    EXPECT_FALSE(st.body["hasUsers"].get<bool>());
    EXPECT_EQ(f.post("/api/setup", {{"username", "ab"}, {"password", "Admin1234"}}).code(), "E010");
    EXPECT_EQ(f.post("/api/setup", {{"username", "admin01"}, {"password", "Admin1234"}}).status, 201);
    EXPECT_EQ(f.post("/api/setup", {{"username", "admin02"}, {"password", "Admin1234"}}).code(), "E004");
    EXPECT_TRUE(ApiWorld::wrap(f.http->Get("/api/status")).body["hasUsers"].get<bool>());
    EXPECT_EQ(f.post("/api/login", {{"username", "admin01"}, {"password", "Admin1234"}}).status, 200);
    EXPECT_EQ(f.post("/api/recovery/restore", {{"username", "admin01"}, {"password", "Admin1234"}}).code(), "E032");   // data not damaged
}

// TC-WEB-02: user, password and audit routes refuse bad input with the right code
TEST(SF1Paths, TC_WEB_02_AccountRoutesRefuseBadInput) {
    ApiWorld a;
    const std::string admin = a.login("admin01", "Admin1234"), rec = a.login("rec01", "Rec@1234");
    EXPECT_EQ(a.post("/api/users", {{"username", "nurse01"}, {"password", "Nurse1234"}, {"role", "Nurse"}}, admin).code(), "E010");
    auto doc = a.post("/api/users", {{"username", "doc03"}, {"password", "Doc12345"}, {"role", "Doctor"}}, admin);
    auto adm = a.post("/api/users", {{"username", "admin02"}, {"password", "Admin1234"}, {"role", "Administrator"}}, admin);
    ASSERT_EQ(doc.status, 201);
    ASSERT_EQ(adm.status, 201);
    EXPECT_EQ(a.post("/api/users", {{"username", "x"}, {"password", "Admin1234"}, {"role", "Doctor"}}, admin).code(), "E010");
    EXPECT_EQ(a.post("/api/users/U99999/status", {{"active", false}}, admin).code(), "E023");
    EXPECT_EQ(a.post("/api/users/U00001/status", {{"active", true}}, rec).code(), "E004");
    std::set<std::string> roles;
    for (const auto& u : a.get("/api/users", admin).body) roles.insert(u["role"].get<std::string>());
    EXPECT_EQ(roles, (std::set<std::string>{"Administrator", "Receptionist", "Doctor"}));
    auto d = a.post("/api/login", {{"username", "doc03"}, {"password", "Doc12345"}});
    EXPECT_EQ(d.body["user"]["role"], "Doctor");
    EXPECT_EQ(d.body["user"]["doctorId"], "");                                                // no doctor profile linked yet
    EXPECT_EQ(a.post("/api/password", {{"current", "Wrong1234"}, {"new", "NewPass99"}}, rec).code(), "E001");
    EXPECT_EQ(a.get("/api/audit", admin).status, 200);                                         // today by default
    EXPECT_EQ(a.get("/api/audit?from=2030-13-01&to=2030-12-31", admin).code(), "E010");
    EXPECT_EQ(a.post("/api/login", {{"username", "rec01"}}).code(), "E001");                  // password missing
}
