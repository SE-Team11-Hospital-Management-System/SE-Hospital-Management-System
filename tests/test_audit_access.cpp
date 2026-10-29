// Member 1 (SF-1) - TC-AUTH-03 role enforcement in every service, TC-SEC-03 audit completeness and content
// (HMS-SEC-002, HMS-SEC-003, HMS-F-042)
#include "security/access_control.h"
#include "test_helpers.h"
#include <fstream>
#include <functional>
#include <gtest/gtest.h>
#include <set>
#include <sstream>

using namespace hms;

namespace {
bool hasEntry(AppContext& ctx, const std::string& action, const std::string& target = "") {
    for (const auto& e : ctx.audit.readAll())
        if (e.action == action && (target.empty() || e.targetId == target)) return true;
    return false;
}
}  // namespace

// TC-AUTH-03: direct service calls with a forbidden role return E004 and write ACCESS_DENIED
TEST(RoleEnforcement, TC_AUTH_03_EveryServiceDeniesForbiddenRoles) {
    hms_test::World w;
    AppContext& c = *w.ctx;
    const std::string appt = c.appointments.book(w.rec, w.p1, w.d1, "2030-01-07 09:00").value();
    struct Case { const char* what; std::function<ErrorCode()> call; };
    const std::vector<Case> cases = {
        {"history (Receptionist)", [&] { return c.consultations.history(w.rec, w.p1).error().code; }},
        {"history (Administrator)", [&] { return c.consultations.history(w.admin, w.p1).error().code; }},
        {"record (Receptionist)", [&] { return c.consultations.record(w.rec, appt, {"Flu", {}, ""}).error().code; }},
        {"backup (Receptionist)", [&] { return c.backup.createBackup(w.rec).error().code; }},
        {"restore (Doctor)", [&] { return c.backup.restore(w.doc1, "20300101_000000", true).error().code; }},
        {"createUser (Receptionist)", [&] { return c.auth.createUser(w.rec, "hack01", "Hack1234", Role::Admin).error().code; }},
        {"register (Doctor)", [&] { return c.patients.registerPatient(w.doc1, {"X Y", "2000-01-01", "M", "9000000000", "A", "O+"}).error().code; }},
        {"deactivate (Receptionist)", [&] { return c.patients.deactivatePatient(w.rec, w.p1).error().code; }},
        {"addDoctor (Receptionist)", [&] { return c.doctors.addDoctor(w.rec, {"X Y", "ENT", "ENT", "U00003"}).error().code; }},
        {"book (Doctor)", [&] { return c.appointments.book(w.doc1, w.p1, w.d1, "2030-01-07 10:00").error().code; }},
        {"cancel (Doctor)", [&] { return c.appointments.cancel(w.doc1, appt).error().code; }},
        {"schedule of D001 (Doctor D002)", [&] { return c.appointments.schedule(w.doc2, w.d1, "2030-01-07").error().code; }},
        {"daily report (Doctor)", [&] { return c.reports.daily(w.doc1, "2030-01-07").error().code; }},
        {"workload (Receptionist)", [&] { return c.reports.workload(w.rec, "2030-01-01", "2030-01-31").error().code; }},
    };
    for (const auto& k : cases) EXPECT_EQ(k.call(), ErrorCode::E004) << k.what;
    size_t denied = 0;
    for (const auto& e : c.audit.readAll()) denied += e.action == "ACCESS_DENIED" && e.outcome == "FAILURE";
    EXPECT_EQ(denied, cases.size());
    EXPECT_EQ(c.appointments.get(appt)->status, AppointmentStatus::Scheduled);   // nothing changed
}

// TC-AUTH-03 (menu part): the main menu shows exactly the functions of SRS Table 6.5-1
TEST(RoleEnforcement, TC_AUTH_03_MenuMatrix) {
    auto allowed = [](Role r) {
        std::set<int> ops;
        for (int op = 0; op <= static_cast<int>(Operation::Backup); ++op)
            if (AccessControl::isAllowed(r, static_cast<Operation>(op))) ops.insert(op);
        return ops;
    };
    auto o = [](Operation op) { return static_cast<int>(op); };
    const std::set<int> doctor = {o(Operation::Login), o(Operation::ChangeOwnPassword), o(Operation::SearchPatient),
                                  o(Operation::ListDoctors), o(Operation::ViewSchedule), o(Operation::RecordConsultation),
                                  o(Operation::ViewMedicalHistory)};
    EXPECT_EQ(allowed(Role::Doctor), doctor);
    EXPECT_EQ(allowed(Role::Receptionist).count(o(Operation::ViewMedicalHistory)), 0u);
    EXPECT_EQ(allowed(Role::Admin).count(o(Operation::RecordConsultation)), 0u);
    EXPECT_EQ(allowed(Role::Receptionist).count(o(Operation::ViewAuditLog)), 0u);
}

// TC-SEC-03: one entry per action with ISO time, user, action, target, outcome; no passwords, no clinical text
TEST(AuditCompleteness, TC_SEC_03_EntriesAndContent) {
    hms_test::World w;
    AppContext& c = *w.ctx;
    (void)c.auth.login("rec01", "Wrong9999");                                    // failed login
    ASSERT_TRUE(c.patients.updatePatient(w.rec, w.p1, {"", "", "", "9000000001", "", ""}).ok());
    ASSERT_TRUE(c.patients.deactivatePatient(w.admin, w.p2).ok());
    const std::string appt = c.appointments.book(w.rec, w.p1, w.d1, "2030-01-07 09:00").value();
    const std::string consult = c.consultations.record(w.doc1, appt, {"Secret diagnosis text", {{"Drug", "1 tab", 3}}, "Private notes"}).value();
    ASSERT_TRUE(c.consultations.history(w.doc1, w.p1).ok());
    const std::string backup = c.backup.createBackup(w.admin).value();
    ASSERT_TRUE(c.backup.restore(w.admin, backup, true).ok());
    (void)c.backup.createBackup(w.rec);                                            // denied

    EXPECT_TRUE(hasEntry(c, "LOGIN_OK"));
    EXPECT_TRUE(hasEntry(c, "LOGIN_FAIL"));
    EXPECT_TRUE(hasEntry(c, "USER_CREATE"));
    EXPECT_TRUE(hasEntry(c, "PATIENT_CREATE", w.p1));
    EXPECT_TRUE(hasEntry(c, "PATIENT_UPDATE", w.p1 + ":phone"));
    EXPECT_TRUE(hasEntry(c, "PATIENT_DEACTIVATE", w.p2));
    EXPECT_TRUE(hasEntry(c, "CONSULT_SAVE", consult));
    EXPECT_TRUE(hasEntry(c, "CONSULT_VIEW", w.p1));
    EXPECT_TRUE(hasEntry(c, "BACKUP", backup));
    EXPECT_TRUE(hasEntry(c, "RESTORE", backup));
    EXPECT_TRUE(hasEntry(c, "ACCESS_DENIED", "backup"));
    for (const auto& e : c.audit.readAll()) {
        ASSERT_EQ(e.timestamp.size(), 19u);                                       // YYYY-MM-DDTHH:MM:SS
        EXPECT_EQ(e.timestamp[10], 'T');
        EXPECT_FALSE(e.userId.empty());
        EXPECT_TRUE(e.outcome == "SUCCESS" || e.outcome == "FAILURE");
    }
    std::ifstream in(c.audit.path());
    std::stringstream ss; ss << in.rdbuf();
    const std::string raw = ss.str();
    for (const char* secret : {"Admin1234", "Rec@1234", "Doc12345", "Wrong9999", "Secret diagnosis", "Private notes", "Drug"})
        EXPECT_EQ(raw.find(secret), std::string::npos) << secret;
}

// Lock-out is audit-logged too (HMS-SEC-003)
TEST(AuditCompleteness, LockoutLogged) {
    hms_test::World w;
    for (int i = 0; i < 3; ++i) (void)w.ctx->auth.login("rec01", "Wrong9999");
    EXPECT_TRUE(hasEntry(*w.ctx, "ACCOUNT_LOCKED"));
}
