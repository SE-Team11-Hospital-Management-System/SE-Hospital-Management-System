// One function per feature menu. Each owner implements only their own file:
//   app_shell.cpp        -> login, role menu, user accounts         (Member 1, SF-1)
//   patient_menu.cpp     -> patients                               (Member 2, SF-2)
//   doctor_menu.cpp      -> doctors and availability               (Member 3, SF-3)
//   appointment_menu.cpp -> appointments                           (Member 3, SF-3)
//   backup_menu.cpp      -> backup and restore                     (Member 4, SF-4)
#pragma once
#include "app/app_context.h"

namespace hms {
int runApp(AppContext& ctx);                                   // whole program after start-up; returns exit code
void userMenu(AppContext& ctx, const Session& s);
void patientMenu(AppContext& ctx, const Session& s);
void doctorMenu(AppContext& ctx, const Session& s);
void appointmentMenu(AppContext& ctx, const Session& s);
void backupMenu(AppContext& ctx, const Session& s);
}  // namespace hms
