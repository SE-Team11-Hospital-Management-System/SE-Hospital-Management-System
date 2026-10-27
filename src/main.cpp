// HMS server. Usage: hms [--port 8080] [--data data] [--web web/dist]
// Starts the web server on this computer only (127.0.0.1) and serves the website and its data.
#include "api/routes.h"
#include "app/bootstrap.h"
#include <cstdlib>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    using namespace hms;
    std::string dataDir = "data", webDir = "web/dist";
    int port = 8080;
    for (int i = 1; i + 1 < argc; i += 2) {
        const std::string k = argv[i], v = argv[i + 1];
        if (k == "--port") port = std::atoi(v.c_str());
        else if (k == "--data") dataDir = v;
        else if (k == "--web") webDir = v;
    }
    AppContext ctx(dataDir, "backup", "reports");
    Api api(ctx, webDir);
    registerAllRoutes(api);
    auto loaded = loadAllData(ctx);
    if (!loaded.ok() && loaded.error().code == ErrorCode::E040) {
        api.enterRecovery(loaded.error());                    // HMS-SEC-007: the website offers the restore
        std::cout << "ERROR E040: Data file damaged. Open the website to restore the latest backup.\n";
    } else if (!loaded.ok()) {
        std::cout << "ERROR " << codeName(loaded.error().code) << ": " << loaded.error().message << '\n';
        return 2;
    }
    if (api.bind(Api::kHost, port) < 0) {                       // this computer only: never reachable from the network
        std::cout << "Port " << port << " is already in use. Close the other HMS window, or use --port 8081.\n";
        return 3;
    }
    std::cout << "HMS is running. Open http://localhost:" << port << " in your browser.\n"
              << "Press Ctrl+C in this window to stop it.\n" << std::flush;
    api.serve();
    return 0;
}
