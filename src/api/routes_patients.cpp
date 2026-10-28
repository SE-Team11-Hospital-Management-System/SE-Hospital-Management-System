// Patients: search, view, register (with duplicate check), update, deactivate (HMS-F-010..015, SEC-005).
#include "api/routes.h"
#include <cstdlib>

namespace hms {

namespace {
PatientInput inputFrom(const Call& c) {
    return PatientInput{c.str("fullName"), c.str("dob"), c.str("gender"), c.str("phone"), c.str("address"), c.str("bloodGroup")};
}
}  // namespace

void registerPatientRoutes(Api& api) {
    AppContext& ctx = api.ctx();
    api.get("/api/patients", [&ctx](Call& c) {                  // ?by=id|name|phone&term=...&page=1
        const std::string by = c.query("by");
        const SearchKey key = by == "id" ? SearchKey::Id : by == "phone" ? SearchKey::Phone : SearchKey::Name;
        const int page = c.query("page").empty() ? 1 : std::atoi(c.query("page").c_str());
        auto r = ctx.patients.search(c.session, key, c.query("term"), page);
        if (!r.ok()) { c.fail(r.error()); return; }
        json items = json::array();
        for (const auto& p : r.value().items) items.push_back(toJson(p));
        c.ok({{"items", items}, {"page", page}, {"hasNext", r.value().hasNext}});
    });
    api.get("/api/patients/:id", [&ctx](Call& c) {
        auto r = ctx.patients.search(c.session, SearchKey::Id, c.param("id"));
        if (!r.ok()) { c.fail(r.error()); return; }
        if (r.value().items.empty()) { c.fail(makeError(ErrorCode::E020, "Patient not found.")); return; }
        c.ok(toJson(r.value().items.front()));
    });
    api.post("/api/patients", [&ctx](Call& c) {                 // E011 + existing ID when a duplicate exists (HMS-F-015)
        auto r = ctx.patients.registerPatient(c.session, inputFrom(c), c.body.value("confirmDuplicate", false));
        if (!r.ok()) { c.fail(r.error()); return; }
        c.ok(toJson(*ctx.patients.get(r.value())), 201);
    });
    api.put("/api/patients/:id", [&ctx](Call& c) {              // ID and date of birth never change (HMS-F-013)
        auto r = ctx.patients.updatePatient(c.session, c.param("id"), inputFrom(c));
        if (!r.ok()) { c.fail(r.error()); return; }
        c.ok(toJson(*ctx.patients.get(c.param("id"))));
    });
    api.post("/api/patients/:id/deactivate", [&ctx](Call& c) {  // HMS-F-014: Administrator only, never deleted
        auto r = ctx.patients.deactivatePatient(c.session, c.param("id"));
        if (!r.ok()) { c.fail(r.error()); return; }
        c.ok(toJson(*ctx.patients.get(c.param("id"))));
    });
}
}  // namespace hms
