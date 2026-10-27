// Login and logout through the website (HMS-F-001, SEC-008). Password, users and audit routes follow this week.
#include "api/routes.h"

namespace hms {

namespace {
json userInfo(AppContext& ctx, const Session& s) {
    auto u = ctx.auth.findUser(s.userId);
    return {{"userId", s.userId}, {"username", u ? u->username : ""}, {"role", roleName(s.role)},
            {"doctorId", s.doctorId}, {"permissions", permissionsFor(s.role)}};
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
}
}  // namespace hms
