// Login, logout, change password, user accounts, audit log viewer (HMS-F-001..006, F-062, SEC-004, SEC-008).
#include "api/routes.h"
#include "common/datetime.h"
#include "security/access_control.h"
#include "security/validator.h"

namespace hms {

namespace {
json userInfo(AppContext& ctx, const Session& s) {
    auto u = ctx.auth.findUser(s.userId);
    return {{"userId", s.userId}, {"username", u ? u->username : ""}, {"role", roleName(s.role)},
            {"doctorId", s.doctorId}, {"permissions", permissionsFor(s.role)}};
}

Role roleFrom(const std::string& name) {
    if (name == "Administrator") return Role::Admin;
    if (name == "Doctor") return Role::Doctor;
    return Role::Receptionist;
}

void registerUserRoutes(Api& api, AppContext& ctx) {        // HMS-F-004 (Administrator only)
    api.get("/api/users", [&ctx](Call& c) {
        auto r = ctx.auth.listUsers(c.session);
        if (!r.ok()) { c.fail(r.error()); return; }
        json out = json::array();
        for (const auto& u : r.value()) out.push_back(toJson(u));
        c.ok(out);
    });
    api.post("/api/users", [&ctx](Call& c) {
        const std::string role = c.str("role");
        if (role != "Administrator" && role != "Receptionist" && role != "Doctor") { c.fail(makeError(ErrorCode::E010, "Role: Administrator, Receptionist or Doctor.")); return; }
        auto r = ctx.auth.createUser(c.session, c.str("username"), c.str("password"), roleFrom(role));
        if (!r.ok()) { c.fail(r.error()); return; }
        c.ok({{"userId", r.value()}}, 201);
    });
    api.post("/api/users/:id/status", [&ctx](Call& c) {
        const bool active = c.body.value("active", false);
        auto r = ctx.auth.setUserStatus(c.session, c.param("id"), active);
        if (!r.ok()) { c.fail(r.error()); return; }
        c.ok({{"userId", c.param("id")}, {"active", active}});
    });
}

void registerAuditRoute(Api& api, AppContext& ctx) {        // HMS-F-062 (Administrator only, read-only)
    api.get("/api/audit", [&ctx](Call& c) {
        if (auto ok = AccessControl::authorize(c.session, Operation::ViewAuditLog, "viewAuditLog"); !ok.ok()) { c.fail(ok.error()); return; }
        std::string from = c.query("from"), to = c.query("to");
        if (from.empty()) from = dt::today();
        if (to.empty()) to = from;
        if (!Validator::isValidDate(from) || !Validator::isValidDate(to)) { c.fail(makeError(ErrorCode::E010, "Date: must be a real date in YYYY-MM-DD format.")); return; }
        if (to < from) { c.fail(makeError(ErrorCode::E010, "Date range: the end date must not be before the start date.")); return; }
        std::string userId;
        if (const std::string name = c.query("username"); !name.empty()) {
            const auto users = ctx.auth.listUsers(c.session);
            if (users.ok())
                for (const auto& u : users.value()) if (u.username == name) userId = u.userId;
            if (userId.empty()) { c.fail(makeError(ErrorCode::E023, "User not found.")); return; }
        }
        json out = json::array();
        for (const auto& e : ctx.audit.query(from, to, userId)) out.push_back(toJson(e));
        c.ok(out);
    });
}
}  // namespace

void registerAuthRoutes(Api& api) {
    AppContext& ctx = api.ctx();
    api.post("/api/login", [&ctx](Call& c) {                    // HMS-SEC-008: one generic message for every failure
        auto r = ctx.auth.login(c.str("username"), c.str("password"));
        if (!r.ok()) { c.fail(r.error()); return; }
        Session s = r.value();
        if (s.role == Role::Doctor)
            if (auto d = ctx.doctors.findByUser(s.userId)) { s.doctorId = d->doctorId; ctx.sessions.attachDoctor(s.token, s.doctorId); }
        c.ok({{"token", s.token}, {"user", userInfo(ctx, s)}});
    }, false);
    api.post("/api/logout", [&ctx](Call& c) {
        ctx.auth.logout(c.session);
        c.ok({{"loggedOut", true}});
    });
    api.get("/api/me", [&ctx](Call& c) { c.ok(userInfo(ctx, c.session)); });
    api.post("/api/password", [&ctx](Call& c) {                 // HMS-F-006
        auto r = ctx.auth.changePassword(c.session, c.str("current"), c.str("new"));
        if (!r.ok()) { c.fail(r.error()); return; }
        c.ok({{"changed", true}});
    });
    registerUserRoutes(api, ctx);
    registerAuditRoute(api, ctx);
}
}  // namespace hms
