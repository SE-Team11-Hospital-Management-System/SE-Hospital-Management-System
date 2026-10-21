// Backup and restore screens (HMS-F-061). Administrator only.
#include "app/bootstrap.h"
#include "ui/console_ui.h"
#include "ui/menus.h"
#include <iostream>

namespace hms {

void backupMenu(AppContext& ctx, const Session& s) {
    while (!ConsoleUi::inputClosed()) {
        ConsoleUi::header("Backup and restore");
        std::cout << "  1. Create backup now\n  2. List backups\n  3. Restore a backup\n  0. Back\n";
        const int c = ConsoleUi::readChoice("Choice: ");
        if (c == 0) return;
        if (c == 1) {
            auto r = ctx.backup.createBackup(s);
            if (r.ok()) ConsoleUi::printOk("Backup created in " + ctx.backup.backupRoot() + "/" + r.value());
            else ConsoleUi::printError(r.error());
        } else if (c == 2 || c == 3) {
            auto list = ctx.backup.listBackups(s);
            if (!list.ok()) { ConsoleUi::printError(list.error()); continue; }
            if (list.value().empty()) { std::cout << "No backups yet.\n"; continue; }
            for (size_t i = 0; i < list.value().size(); ++i) std::cout << "  " << i + 1 << ". " << list.value()[i] << '\n';
            if (c == 2) continue;
            const int n = ConsoleUi::readChoice("Backup number to restore (0 = cancel): ");
            if (n < 1 || n > static_cast<int>(list.value().size())) continue;
            const std::string name = list.value()[static_cast<size_t>(n - 1)];
            std::cout << "All current data will be replaced by backup " << name << ".\n";
            auto r = ctx.backup.restore(s, name, ConsoleUi::confirm("Are you sure?"));
            if (!r.ok()) { ConsoleUi::printError(r.error()); continue; }
            auto reload = loadAllData(ctx);                              // use the restored data immediately
            if (reload.ok()) ConsoleUi::printOk("Backup " + name + " restored.");
            else ConsoleUi::printError(reload.error());
        } else {
            ConsoleUi::printError("E010", "Choice: enter a number from the menu.");
        }
    }
}
}  // namespace hms
