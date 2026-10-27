// Appointments: book (with free-slot suggestions), list, cancel, no-show, reschedule, daily schedule
// (HMS-F-030..035). A refused slot (E030 / E031) carries the next 3 free slots in error.details, comma separated.
#include "api/routes.h"
#include "common/datetime.h"

namespace hms {

namespace {
json list(AppContext& ctx, const std::vector<Appointment>& v) {     // with the patient's name for the screens
    json out = json::array();
    for (const auto& a : v) {
        json row = toJson(a);
        auto p = ctx.patients.get(a.patientId);
        row["patientName"] = p ? p->fullName : "";
        out.push_back(row);
    }
    return out;
}
void reply(const Call& c, AppContext& ctx, const Result<void>& r, const std::string& id) {
    if (!r.ok()) { c.fail(r.error()); return; }
    c.ok(toJson(*ctx.appointments.get(id)));
}
}  // namespace

void registerAppointmentRoutes(Api& api) {
    AppContext& ctx = api.ctx();
    api.post("/api/appointments", [&ctx](Call& c) {
        auto r = ctx.appointments.book(c.session, c.str("patientId"), c.str("doctorId"), c.str("slot"));
        if (!r.ok()) { c.fail(r.error()); return; }
        c.ok(toJson(*ctx.appointments.get(r.value())), 201);
    });
    api.get("/api/appointments", [&ctx](Call& c) {              // ?patientId=P000001
        auto r = ctx.appointments.forPatient(c.session, c.query("patientId"));
        if (!r.ok()) { c.fail(r.error()); return; }
        c.ok(list(ctx, r.value()));
    });
    api.post("/api/appointments/:id/cancel", [&ctx](Call& c) {
        reply(c, ctx, ctx.appointments.cancel(c.session, c.param("id")), c.param("id"));
    });
    api.post("/api/appointments/:id/no-show", [&ctx](Call& c) {
        reply(c, ctx, ctx.appointments.markNoShow(c.session, c.param("id")), c.param("id"));
    });
    api.post("/api/appointments/:id/reschedule", [&ctx](Call& c) {
        reply(c, ctx, ctx.appointments.reschedule(c.session, c.param("id"), c.str("doctorId"), c.str("slot")), c.param("id"));
    });
    api.get("/api/schedule", [&ctx](Call& c) {                  // ?doctorId=D001&date=YYYY-MM-DD (Doctor: own by default)
        const std::string doctor = c.query("doctorId").empty() ? c.session.doctorId : c.query("doctorId");
        const std::string date = c.query("date").empty() ? dt::today() : c.query("date");
        auto r = ctx.appointments.schedule(c.session, doctor, date);
        if (!r.ok()) { c.fail(r.error()); return; }
        c.ok({{"doctorId", doctor}, {"date", date}, {"appointments", list(ctx, r.value())}});
    });
}
}  // namespace hms
