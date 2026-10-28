// Member 4 (SF-4) - web routes for reports and backups (HMS-F-050, F-051, F-061)
#include "api_helpers.h"
#include <gtest/gtest.h>

using namespace hms;
using hms_test::ApiWorld;

TEST(WebApi, ReportsCsvAndBackups) {
    ApiWorld a;
    const std::string rec = a.login("rec01", "Rec@1234"), admin = a.login("admin01", "Admin1234"), doc = a.login("doc01", "Doc12345");
    a.post("/api/appointments", {{"patientId", a.w.p1}, {"doctorId", "D001"}, {"slot", "2030-01-07 09:00"}}, rec);
    a.post("/api/appointments", {{"patientId", a.w.p2}, {"doctorId", "D002"}, {"slot", "2030-01-07 10:00"}}, rec);
    auto daily = a.get("/api/reports/daily?date=2030-01-07", rec);
    ASSERT_EQ(daily.status, 200);
    EXPECT_EQ(daily.body["total"], 2);
    EXPECT_EQ(daily.body["perStatus"]["SCHEDULED"], 2);
    EXPECT_EQ(daily.body["perDoctor"][0]["appointments"][0]["patientName"], "Anita Rao");
    EXPECT_EQ(a.get("/api/reports/daily?date=2030-01-07", doc).status, 403);
    auto csv = a.http->Get("/api/reports/daily/csv?date=2030-01-07", ApiWorld::bearer(rec));
    ASSERT_TRUE(csv);
    EXPECT_EQ(csv->status, 200);
    EXPECT_EQ(csv->get_header_value("Content-Type"), "text/csv");
    EXPECT_EQ(csv->body.rfind("appointment_id,date,time", 0), 0u);
    EXPECT_EQ(a.get("/api/reports/workload?from=2030-01-31&to=2030-01-01", admin).code(), "E010");
    EXPECT_EQ(a.get("/api/reports/workload?from=2030-01-01&to=2030-01-31", rec).status, 403);

    auto b = a.post("/api/backups", json::object(), admin);
    ASSERT_EQ(b.status, 201);
    const std::string name = b.body["name"];
    EXPECT_EQ(a.get("/api/backups", admin).body[0], name);
    EXPECT_EQ(a.post("/api/backups", json::object(), rec).status, 403);
    EXPECT_EQ(a.post("/api/backups/" + name + "/restore", json::object(), admin).code(), "E010");   // needs confirm
    EXPECT_EQ(a.post("/api/backups/" + name + "/restore", {{"confirm", true}}, admin).status, 200);
}
