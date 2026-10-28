// Restore the latest backup when the data failed its check at start-up (HMS-SEC-007).
#include "api/routes.h"
#include "app/recovery.h"

namespace hms {

void registerRecoveryRoutes(Api& api) {
    AppContext& ctx = api.ctx();
    api.allowInRecovery("/api/recovery/restore");
    api.post("/api/recovery/restore", [&api, &ctx](Call& c) {
        if (!api.inRecovery()) { c.fail(makeError(ErrorCode::E032, "Action not allowed: the data is not damaged.")); return; }
        auto r = restoreLatestBackup(ctx, c.str("username"), c.str("password"));
        if (!r.ok()) { c.fail(r.error()); return; }
        api.leaveRecovery();
        c.ok({{"restored", r.value()}});
    }, false);
}
}  // namespace hms
