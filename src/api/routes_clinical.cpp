// Consultations, Doctors only: patient check before saving, record, history, addendum (HMS-F-040..043, NF-010).
// There is deliberately no route to edit or delete a consultation (HMS-F-043).
#include "api/routes.h"

namespace hms {

namespace {
ConsultationInput inputFrom(const Call& c) {
    ConsultationInput in{c.str("diagnosis"), {}, c.str("notes")};
    if (c.body.contains("items") && c.body["items"].is_array())
        for (const auto& it : c.body["items"]) {
            if (!it.is_object()) continue;
            const json days = it.value("days", json());
            in.items.push_back(PrescriptionItem{it.value("medicine", ""), it.value("dosage", ""), days.is_number_integer() ? days.get<int>() : 0});
        }
    return in;
}
}  // namespace

void registerClinicalRoutes(Api& api) {
    AppContext& ctx = api.ctx();
    api.get("/api/consultations/prepare", [&ctx](Call& c) {    // who the patient is, shown before saving (HMS-NF-010)
        auto r = ctx.consultations.prepare(c.session, c.query("appointmentId"));
        if (!r.ok()) { c.fail(r.error()); return; }
        c.ok({{"patientId", r.value().patientId}, {"fullName", r.value().fullName}, {"age", r.value().age}});
    });
    api.post("/api/consultations", [&ctx](Call& c) {
        if (!c.body.value("confirmed", false)) { c.fail(makeError(ErrorCode::E010, "Confirmation: the patient must be confirmed before saving.")); return; }
        auto r = ctx.consultations.record(c.session, c.str("appointmentId"), inputFrom(c));
        if (!r.ok()) { c.fail(r.error()); return; }
        c.ok({{"consultId", r.value()}}, 201);
    });
    api.get("/api/consultations", [&ctx](Call& c) {            // ?patientId=P000001, newest first, audit-logged
        auto r = ctx.consultations.history(c.session, c.query("patientId"));
        if (!r.ok()) { c.fail(r.error()); return; }
        json out = json::array();
        for (const auto& x : r.value()) out.push_back(toJson(x));
        c.ok(out);
    });
    api.post("/api/consultations/:id/addenda", [&ctx](Call& c) {
        auto r = ctx.consultations.addAddendum(c.session, c.param("id"), c.str("text"));
        if (!r.ok()) { c.fail(r.error()); return; }
        c.ok({{"consultId", c.param("id")}}, 201);
    });
}
}  // namespace hms
