// Member 4 (SF-4) - web routes for consultations (HMS-F-040..043, NF-010)
#include "api_helpers.h"
#include <gtest/gtest.h>

using namespace hms;
using hms_test::ApiWorld;

TEST(WebApi, ConsultationsNeedConfirmationAndStayPrivate) {
    ApiWorld a;
    const std::string rec = a.login("rec01", "Rec@1234"), doc = a.login("doc01", "Doc12345"), admin = a.login("admin01", "Admin1234");
    const std::string appt = a.post("/api/appointments", {{"patientId", a.w.p1}, {"doctorId", "D001"}, {"slot", "2030-01-07 09:00"}}, rec).body["appointmentId"];

    auto who = a.get("/api/consultations/prepare?appointmentId=" + appt, doc);
    ASSERT_EQ(who.status, 200) << who.raw;
    EXPECT_EQ(who.body["fullName"], "Anita Rao");
    const json visit = {{"appointmentId", appt}, {"diagnosis", "Hypertension"}, {"notes", ""},
                        {"items", json::array({{{"medicine", "Amlodipine"}, {"dosage", "5 mg"}, {"days", 30}}})}};
    EXPECT_EQ(a.post("/api/consultations", visit, doc).code(), "E010");                    // not confirmed (HMS-NF-010)
    json ok = visit; ok["confirmed"] = true;
    EXPECT_EQ(a.post("/api/consultations", ok, rec).status, 403);
    auto saved = a.post("/api/consultations", ok, doc);
    ASSERT_EQ(saved.status, 201) << saved.raw;
    const std::string cid = saved.body["consultId"];
    EXPECT_EQ(a.post("/api/consultations/" + cid + "/addenda", {{"text", "Review in 4 weeks"}}, doc).status, 201);
    auto history = a.get("/api/consultations?patientId=" + a.w.p1, doc);
    ASSERT_EQ(history.status, 200);
    ASSERT_EQ(history.body.size(), 1u);
    EXPECT_EQ(history.body[0]["items"][0]["medicine"], "Amlodipine");
    EXPECT_EQ(history.body[0]["addenda"][0]["text"], "Review in 4 weeks");
    EXPECT_EQ(a.get("/api/consultations?patientId=" + a.w.p1, rec).status, 403);          // HMS-F-042
    EXPECT_EQ(a.get("/api/consultations?patientId=" + a.w.p1, admin).status, 403);
}
