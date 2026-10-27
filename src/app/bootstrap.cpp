#include "app/bootstrap.h"
#include "data/file_store.h"
#include <chrono>

namespace hms {

Result<LoadReport> loadAllData(AppContext& ctx) {
    const auto t0 = std::chrono::steady_clock::now();
    if (auto d = FileStore::ensureDirectory(ctx.dataDir); !d.ok()) return d.error();   // owner-only folder (HMS-SEC-006)
    if (auto r = ctx.auth.load(); !r.ok()) return r.error();
    if (auto r = ctx.patients.load(); !r.ok()) return r.error();
    if (auto r = ctx.doctors.load(); !r.ok()) return r.error();
    if (auto r = ctx.appointments.load(); !r.ok()) return r.error();
    if (auto r = ctx.consultations.load(); !r.ok()) return r.error();
    LoadReport rep;
    rep.patients = ctx.patientRepo.size();
    rep.doctors = ctx.doctors.count();
    rep.appointments = ctx.appointments.all().size();
    rep.seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    return rep;
}
}  // namespace hms
