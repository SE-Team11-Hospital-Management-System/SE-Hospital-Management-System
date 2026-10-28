// Member 2 (SF-2) - web routes for patients (HMS-F-010..015)
#include "api_helpers.h"
#include <gtest/gtest.h>

using namespace hms;
using hms_test::ApiWorld;

TEST(WebApi, PatientsRegisterSearchUpdateDeactivate) {
    ApiWorld a;
    const std::string rec = a.login("rec01", "Rec@1234"), admin = a.login("admin01", "Admin1234"), doc = a.login("doc01", "Doc12345");
    const json priya = {{"fullName", "Priya Sharma"}, {"dob", "1995-03-02"}, {"gender", "F"}, {"phone", "9988776655"},
                        {"address", "Indiranagar"}, {"bloodGroup", "A+"}};
    auto created = a.post("/api/patients", priya, rec);
    ASSERT_EQ(created.status, 201) << created.raw;
    EXPECT_EQ(created.body["patientId"], "P000003");
    auto dup = a.post("/api/patients", priya, rec);
    EXPECT_EQ(dup.status, 409);
    EXPECT_EQ(dup.body["error"]["details"], "P000003");                                    // existing ID shown
    json confirmed = priya; confirmed["confirmDuplicate"] = true;
    EXPECT_EQ(a.post("/api/patients", confirmed, rec).status, 201);
    json bad = priya; bad["phone"] = "98765";
    EXPECT_EQ(a.post("/api/patients", bad, rec).body["error"]["message"], "Phone: must be exactly 10 digits.");
    EXPECT_EQ(a.post("/api/patients", priya, doc).status, 403);                           // Doctor cannot register

    auto found = a.get("/api/patients?by=name&term=priya", doc);
    ASSERT_EQ(found.status, 200);
    EXPECT_EQ(found.body["items"].size(), 2u);
    EXPECT_FALSE(found.body["hasNext"].get<bool>());
    EXPECT_EQ(a.get("/api/patients/P000003", rec).body["fullName"], "Priya Sharma");
    EXPECT_EQ(a.get("/api/patients/P999999", rec).code(), "E020");

    auto updated = a.put("/api/patients/P000003", {{"phone", "9000000009"}}, rec);
    ASSERT_EQ(updated.status, 200);
    EXPECT_EQ(updated.body["phone"], "9000000009");
    EXPECT_EQ(updated.body["dob"], "1995-03-02");
    EXPECT_EQ(a.post("/api/patients/P000003/deactivate", json::object(), rec).status, 403);
    auto off = a.post("/api/patients/P000003/deactivate", json::object(), admin);
    ASSERT_EQ(off.status, 200);
    EXPECT_FALSE(off.body["active"].get<bool>());
}
