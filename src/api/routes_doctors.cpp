// Doctors: list with filter, add, update, weekly availability (HMS-F-020/021/022).
#include "api/routes.h"
#include <optional>

namespace hms {

namespace {
DoctorInput inputFrom(const Call& c) { return DoctorInput{c.str("fullName"), c.str("specialization"), c.str("department"), c.str("userId")}; }

bool windowsFrom(const json& body, std::vector<Window>& out) {
    if (!body.contains("windows") || !body["windows"].is_array()) return false;
    for (const auto& w : body["windows"]) {
        if (!w.is_object() || !w.value("weekday", json()).is_number_integer()) return false;
        out.push_back(Window{w["weekday"].get<int>(), w.value("start", ""), w.value("end", "")});
    }
    return true;
}
}  // namespace

void registerDoctorRoutes(Api& api) {
    AppContext& ctx = api.ctx();
    api.get("/api/doctors", [&ctx](Call& c) {
        const std::string spec = c.query("specialization");
        auto r = ctx.doctors.list(c.session, spec.empty() ? std::nullopt : std::optional<std::string>(spec));
        if (!r.ok()) { c.fail(r.error()); return; }
        json out = json::array();
        for (const auto& d : r.value()) out.push_back(toJson(d));
        c.ok(out);
    });
    api.post("/api/doctors", [&ctx](Call& c) {
        auto r = ctx.doctors.addDoctor(c.session, inputFrom(c));
        if (!r.ok()) { c.fail(r.error()); return; }
        c.ok(toJson(*ctx.doctors.get(r.value())), 201);
    });
    api.put("/api/doctors/:id", [&ctx](Call& c) {
        auto r = ctx.doctors.updateDoctor(c.session, c.param("id"), inputFrom(c));
        if (!r.ok()) { c.fail(r.error()); return; }
        c.ok(toJson(*ctx.doctors.get(c.param("id"))));
    });
    api.put("/api/doctors/:id/availability", [&ctx](Call& c) {
        std::vector<Window> ws;
        if (!windowsFrom(c.body, ws)) { c.fail(makeError(ErrorCode::E010, "Working hours: a list of {weekday, start, end}.")); return; }
        auto r = ctx.doctors.setAvailability(c.session, c.param("id"), ws);
        if (!r.ok()) { c.fail(r.error()); return; }
        c.ok(toJson(*ctx.doctors.get(c.param("id"))));
    });
}
}  // namespace hms
