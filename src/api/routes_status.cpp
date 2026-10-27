// Public routes used before login: is this the first run, is the data damaged, create the first Administrator.
#include "api/routes.h"
#include "security/access_control.h"
#include <filesystem>

namespace hms {

json permissionsFor(Role role) {
    static const std::pair<Operation, const char*> ops[] = {
        {Operation::ChangeOwnPassword, "changePassword"}, {Operation::ManageUsers, "manageUsers"},
        {Operation::ViewAuditLog, "viewAuditLog"}, {Operation::RegisterPatient, "registerPatient"},
        {Operation::UpdatePatient, "updatePatient"}, {Operation::SearchPatient, "searchPatient"},
        {Operation::DeactivatePatient, "deactivatePatient"}, {Operation::ManageDoctors, "manageDoctors"},
        {Operation::ListDoctors, "listDoctors"}, {Operation::BookAppointment, "bookAppointment"},
        {Operation::ViewSchedule, "viewSchedule"}, {Operation::RecordConsultation, "recordConsultation"},
        {Operation::ViewMedicalHistory, "viewMedicalHistory"}, {Operation::DailyReport, "dailyReport"},
        {Operation::WorkloadReport, "workloadReport"}, {Operation::Backup, "backup"}};
    json out = json::array();
    for (const auto& [op, name] : ops)
        if (AccessControl::isAllowed(role, op)) out.push_back(name);
    return out;
}

void registerStatusRoutes(Api& api) {
    AppContext& ctx = api.ctx();
    api.allowInRecovery("/api/status");
    api.get("/api/status", [&api, &ctx](Call& c) {
        json recovery = {{"needed", api.inRecovery()}};
        if (api.inRecovery()) {
            recovery["file"] = std::filesystem::path(api.damage().details).filename().string();
            recovery["latestBackup"] = ctx.backup.latestBackup();
        }
        c.ok({{"hasUsers", ctx.auth.hasUsers()}, {"recovery", recovery}});
    }, false);

    api.post("/api/setup", [&ctx](Call& c) {                     // SRS 2.6: first Administrator, first run only
        auto r = ctx.auth.bootstrapAdmin(c.str("username"), c.str("password"));
        if (!r.ok()) { c.fail(r.error()); return; }
        c.ok({{"userId", r.value()}}, 201);
    }, false);
}
}  // namespace hms
