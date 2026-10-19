// SKELETON STUB (TASK-02). Replaced by the owner's pull request during Sprint 1 - see the header for the owner.
// Owner: Member 4 (Shristi). BASIC VERSION: only creates the data folder; loading arrives in PR09.
#include "app/bootstrap.h"
#include "data/file_store.h"
namespace hms {
Result<LoadReport> loadAllData(AppContext& ctx) {
    if (auto d = FileStore::ensureDirectory(ctx.dataDir); !d.ok()) return d.error();
    return LoadReport{};
}
}  // namespace hms
