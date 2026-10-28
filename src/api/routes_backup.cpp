// Backup and restore, Administrator only (HMS-F-061). After a restore all data is loaded again at once.
#include "api/routes.h"
#include "app/bootstrap.h"

namespace hms {

void registerBackupRoutes(Api& api) {
    AppContext& ctx = api.ctx();
    api.get("/api/backups", [&ctx](Call& c) {
        auto r = ctx.backup.listBackups(c.session);
        if (!r.ok()) { c.fail(r.error()); return; }
        c.ok(r.value());
    });
    api.post("/api/backups", [&ctx](Call& c) {
        auto r = ctx.backup.createBackup(c.session);
        if (!r.ok()) { c.fail(r.error()); return; }
        c.ok({{"name", r.value()}}, 201);
    });
    api.post("/api/backups/:name/restore", [&ctx](Call& c) {
        auto r = ctx.backup.restore(c.session, c.param("name"), c.body.value("confirm", false));
        if (!r.ok()) { c.fail(r.error()); return; }
        auto reload = loadAllData(ctx);
        if (!reload.ok()) { c.fail(reload.error()); return; }
        c.ok({{"restored", c.param("name")}});
    });
}
}  // namespace hms
