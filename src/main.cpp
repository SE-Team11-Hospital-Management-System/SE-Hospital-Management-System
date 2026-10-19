// HMS entry point. Usage: hms [dataFolder]   (default: ./data, backups go to ./backup)
#include "app/app_context.h"
#include "app/bootstrap.h"
#include "ui/console_ui.h"
#include "ui/menus.h"
#include <iostream>

int main(int argc, char** argv) {
    using namespace hms;
    const std::string dataDir = argc > 1 ? argv[1] : "data";
    AppContext ctx(dataDir, "backup");
    auto loaded = loadAllData(ctx);
    if (!loaded.ok()) {
        ConsoleUi::printError(loaded.error());
        std::cout << "Ask the Administrator to restore the latest backup (folder 'backup').\n";
        return 2;
    }
    return runApp(ctx);
}
