// Member 3 (SF-3) - web routes for doctors, appointments and schedules (HMS-F-020..022, F-030..035)
#include "api_helpers.h"
#include <gtest/gtest.h>

using namespace hms;
using hms_test::ApiWorld;

TEST(WebApi, DoctorsAppointmentsAndSchedule) {
    ApiWorld a;
    const std::string rec = a.login("rec01", "Rec@1234"), admin = a.login("admin01", "Admin1234");
    const std::string doc1 = a.login("doc01", "Doc12345"), doc2 = a.login("doc02", "Doc12345");

    auto list = a.get("/api/doctors?specialization=Cardiology", rec);
    ASSERT_EQ(list.status, 200);
    ASSERT_EQ(list.body.size(), 1u);
    EXPECT_EQ(list.body[0]["windows"].size(), 7u);
    EXPECT_EQ(a.put("/api/doctors/D001", {{"department", "Heart Centre"}}, rec).status, 403);
    EXPECT_EQ(a.put("/api/doctors/D001", {{"department", "Heart Centre"}}, admin).body["department"], "Heart Centre");
    auto hours = a.put("/api/doctors/D002/availability", {{"windows", json::array({{{"weekday", 1}, {"start", "09:00"}, {"end", "12:00"}}})}}, admin);
    ASSERT_EQ(hours.status, 200) << hours.raw;
    EXPECT_EQ(a.put("/api/doctors/D002/availability", {{"windows", "nope"}}, admin).code(), "E010");

    auto booked = a.post("/api/appointments", {{"patientId", a.w.p1}, {"doctorId", "D001"}, {"slot", "2030-01-07 09:00"}}, rec);
    ASSERT_EQ(booked.status, 201) << booked.raw;
    const std::string id = booked.body["appointmentId"];
    EXPECT_EQ(booked.body["status"], "SCHEDULED");
    auto taken = a.post("/api/appointments", {{"patientId", a.w.p2}, {"doctorId", "D001"}, {"slot", "2030-01-07 09:00"}}, rec);
    EXPECT_EQ(taken.status, 409);
    EXPECT_EQ(taken.code(), "E030");
    EXPECT_EQ(taken.body["error"]["details"], "2030-01-07 09:30, 2030-01-07 10:00, 2030-01-07 10:30");

    auto moved = a.post("/api/appointments/" + id + "/reschedule", {{"doctorId", "D001"}, {"slot", "2030-01-07 11:00"}}, rec);
    ASSERT_EQ(moved.status, 200) << moved.raw;
    EXPECT_EQ(moved.body["slotStart"], "2030-01-07 11:00");
    auto mine = a.get("/api/schedule?date=2030-01-07", doc1);                              // Doctor: own schedule by default
    ASSERT_EQ(mine.status, 200);
    EXPECT_EQ(mine.body["appointments"].size(), 1u);
    EXPECT_EQ(a.get("/api/schedule?doctorId=D001&date=2030-01-07", doc2).status, 403);
    EXPECT_EQ(a.get("/api/appointments?patientId=" + a.w.p1, rec).body.size(), 1u);
    EXPECT_EQ(a.post("/api/appointments/" + id + "/no-show", json::object(), rec).code(), "E032");   // not started yet
    EXPECT_EQ(a.post("/api/appointments/" + id + "/cancel", json::object(), rec).body["status"], "CANCELLED");
    EXPECT_EQ(a.post("/api/appointments/" + id + "/cancel", json::object(), rec).code(), "E032");
}
