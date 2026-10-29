// Member 1 (SF-1) - TC-SEC-07: local-only server and unguessable session tokens (HMS-SEC-009)
#include "api_helpers.h"
#include <cctype>
#include <gtest/gtest.h>
#include <set>

using namespace hms;
using hms_test::ApiWorld;

TEST(WebSecurity, TC_SEC_07_TokensAreRandomAndChecked) {
    ApiWorld a;
    std::set<std::string> tokens;
    for (int i = 0; i < 5; ++i) {
        const std::string t = a.login("rec01", "Rec@1234");
        ASSERT_EQ(t.size(), 64u);                                                      // 256-bit hex
        for (char c : t) EXPECT_TRUE(std::isxdigit(static_cast<unsigned char>(c)));
        tokens.insert(t);
    }
    EXPECT_EQ(tokens.size(), 5u);                                                      // never repeated
    std::string changed = *tokens.begin();
    changed[10] = changed[10] == 'a' ? 'b' : 'a';
    EXPECT_EQ(a.get("/api/me", changed).code(), "E003");
    EXPECT_EQ(a.get("/api/patients?by=name&term=a").code(), "E003");                    // no token at all
    const std::string t = *tokens.rbegin();
    ASSERT_EQ(a.post("/api/logout", json::object(), t).status, 200);
    EXPECT_EQ(a.get("/api/me", t).code(), "E003");
}

TEST(WebSecurity, TC_SEC_07_ServerListensOnLoopbackOnly) {
    EXPECT_STREQ(Api::kHost, "127.0.0.1");                                             // main() binds only this address
    ApiWorld a;                                                                        // binds Api::kHost like main()
    httplib::Client local(Api::kHost, a.port);
    EXPECT_TRUE(local.Get("/api/status"));
}

TEST(WebSecurity, PasswordsNeverReturned) {
    ApiWorld a;
    const std::string admin = a.login("admin01", "Admin1234");
    const std::string raw = a.get("/api/users", admin).raw + a.get("/api/me", admin).raw;
    EXPECT_EQ(raw.find("Admin1234"), std::string::npos);
    EXPECT_EQ(raw.find("passwordHash"), std::string::npos);
    EXPECT_EQ(raw.find("salt"), std::string::npos);
}
