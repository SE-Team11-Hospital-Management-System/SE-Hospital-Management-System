// Starts the real HMS web server on a free port in the background and talks to it like the website does.
#pragma once
#include "api/routes.h"
#include "test_helpers.h"
#include <chrono>
#include <httplib.h>
#include <thread>

namespace hms_test {

struct ApiWorld {
    World w;                                     // admin01 / rec01 / doc01 / doc02, D001, D002, P000001, P000002
    hms::Api api;
    std::thread thread;
    int port = -1;
    std::unique_ptr<httplib::Client> http;

    explicit ApiWorld(const std::string& webRoot = "") : api(*w.ctx, webRoot) {
        hms::registerAllRoutes(api);
        port = api.bind(hms::Api::kHost, 0);
        thread = std::thread([this] { api.serve(); });
        http = std::make_unique<httplib::Client>(hms::Api::kHost, port);
        http->set_read_timeout(30, 0);
        for (int i = 0; i < 100 && !http->Get("/api/status"); ++i) std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    ~ApiWorld() { api.stop(); if (thread.joinable()) thread.join(); }
    ApiWorld(const ApiWorld&) = delete;
    ApiWorld& operator=(const ApiWorld&) = delete;

    // Logs in through the API and returns the token.
    std::string login(const std::string& user, const std::string& pwd) {
        auto r = http->Post("/api/login", hms::json{{"username", user}, {"password", pwd}}.dump(), "application/json");
        if (!r || r->status != 200) return "";
        return hms::json::parse(r->body)["token"].get<std::string>();
    }
    static httplib::Headers bearer(const std::string& token) { return {{"Authorization", "Bearer " + token}}; }

    struct Reply { int status = 0; hms::json body; std::string raw; std::string code() const { return body.contains("error") ? body["error"]["code"].get<std::string>() : ""; } };
    static Reply wrap(const httplib::Result& r) {
        Reply out;
        if (!r) return out;
        out.status = r->status;
        out.raw = r->body;
        out.body = hms::json::parse(r->body, nullptr, false);
        return out;
    }
    Reply get(const std::string& path, const std::string& token = "") { return wrap(http->Get(path, bearer(token))); }
    Reply post(const std::string& path, const hms::json& body, const std::string& token = "") {
        return wrap(http->Post(path, bearer(token), body.dump(), "application/json"));
    }
    Reply put(const std::string& path, const hms::json& body, const std::string& token = "") {
        return wrap(http->Put(path, bearer(token), body.dump(), "application/json"));
    }
};
}  // namespace hms_test
