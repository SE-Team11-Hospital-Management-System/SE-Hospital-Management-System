// Start-up: create the data folder and load every data file (HMS-F-060, HMS-NF-003).
#pragma once
#include "app/app_context.h"
#include "common/types.h"

namespace hms {
struct LoadReport {
    double seconds{0};
    size_t patients{0}, doctors{0}, appointments{0};
};
Result<LoadReport> loadAllData(AppContext& ctx);
}  // namespace hms
