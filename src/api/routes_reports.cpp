// Reports: daily appointment report, CSV download, doctor workload (HMS-F-050, F-051). No clinical data.
#include "api/routes.h"
#include "common/datetime.h"
#include <fstream>
#include <sstream>

namespace hms {

namespace {
json dailyJson(AppContext& ctx, const DailyReport& r) {
    json perStatus = json::object(), perDoctor = json::array();
    for (const auto& [st, n] : r.perStatus) perStatus[toString(st)] = n;
    for (const auto& [doc, list] : r.perDoctor) {
        auto d = ctx.doctors.get(doc);
        json rows = json::array();
        for (const auto& a : list) {
            json row = toJson(a);
            auto p = ctx.patients.get(a.patientId);
            row["patientName"] = p ? p->fullName : "";
            rows.push_back(row);
        }
        perDoctor.push_back({{"doctorId", doc}, {"doctorName", d ? d->fullName : ""}, {"appointments", rows}});
    }
    return {{"date", r.date}, {"total", r.total}, {"perStatus", perStatus}, {"perDoctor", perDoctor}};
}
}  // namespace

void registerReportRoutes(Api& api) {
    AppContext& ctx = api.ctx();
    api.get("/api/reports/daily", [&ctx](Call& c) {
        auto r = ctx.reports.daily(c.session, c.query("date").empty() ? dt::today() : c.query("date"));
        if (!r.ok()) { c.fail(r.error()); return; }
        c.ok(dailyJson(ctx, r.value()));
    });
    api.get("/api/reports/daily/csv", [&ctx](Call& c) {         // also kept in the reports folder
        auto r = ctx.reports.daily(c.session, c.query("date").empty() ? dt::today() : c.query("date"));
        if (!r.ok()) { c.fail(r.error()); return; }
        auto path = ctx.reports.exportCsv(c.session, r.value());
        if (!path.ok()) { c.fail(path.error()); return; }
        std::ifstream in(path.value());
        std::stringstream ss; ss << in.rdbuf();
        c.text(ss.str(), "text/csv", "daily_" + r.value().date + ".csv");
    });
    api.get("/api/reports/workload", [&ctx](Call& c) {          // ?from=YYYY-MM-DD&to=YYYY-MM-DD
        auto r = ctx.reports.workload(c.session, c.query("from"), c.query("to"));
        if (!r.ok()) { c.fail(r.error()); return; }
        json out = json::array();
        for (const auto& row : r.value()) out.push_back({{"doctorId", row.doctorId}, {"doctorName", row.doctorName}, {"completed", row.completed}});
        c.ok(out);
    });
}
}  // namespace hms
