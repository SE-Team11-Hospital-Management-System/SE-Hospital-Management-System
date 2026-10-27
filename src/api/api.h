// C15 Web API: a small HTTP server on 127.0.0.1 that the React website talks to (SAD v1.1, section 3).
// Every request runs one at a time (one lock), so the services never run in parallel.
// Answers are JSON. Errors are {"error": {"code": "E010", "message": "...", "details": "..."}}.
#pragma once
#include "app/app_context.h"
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#include <string>

namespace httplib { class Server; struct Request; struct Response; }

namespace hms {

using json = nlohmann::json;

// One HTTP request as seen by a route handler.
struct Call {
    const httplib::Request& req;
    httplib::Response& res;
    Session session;                       // the logged-in user (empty for public routes)
    json body;                             // parsed JSON body ({} if none)

    std::string param(const std::string& name) const;   // ":name" in the route path
    std::string query(const std::string& name) const;   // ?name=... ("" if missing)
    std::string str(const std::string& field) const;    // body["field"] as text ("" if missing or not text)
    void ok(const json& data, int status = 200) const;
    void fail(const Error& e) const;
    void text(const std::string& data, const std::string& contentType, const std::string& fileName = "") const;
};

class Api {
public:
    using Handler = std::function<void(Call&)>;
    Api(AppContext& ctx, std::string webRoot);
    ~Api();

    // Route registration. 'auth' = a valid session is required (E003 otherwise).
    void get(const std::string& path, Handler h, bool auth = true);
    void post(const std::string& path, Handler h, bool auth = true);
    void put(const std::string& path, Handler h, bool auth = true);
    // Allowed even while the data is damaged (status page and restore only).
    void allowInRecovery(const std::string& path) { recoveryAllowed_[path] = true; }

    void enterRecovery(const Error& damage);      // HMS-SEC-007: data failed its check at start-up
    void leaveRecovery() { recovery_ = false; }
    bool inRecovery() const { return recovery_; }
    const Error& damage() const { return damage_; }

    static constexpr const char* kHost = "127.0.0.1";   // HMS-SEC-009: this computer only, never the network
    int bind(const std::string& host, int port);  // returns the port in use (port 0 = any free port), -1 on error
    bool serve();                                 // blocks until stop()
    void stop();
    AppContext& ctx() { return ctx_; }

    static int httpStatus(ErrorCode c);
private:
    void add(const std::string& method, const std::string& path, Handler h, bool auth);
    AppContext& ctx_;
    std::string webRoot_;
    std::unique_ptr<httplib::Server> server_;
    std::mutex lock_;
    bool recovery_{false};
    Error damage_;
    std::map<std::string, bool> recoveryAllowed_;
};

// Shared JSON shapes (camelCase field names, the same names the website uses).
json toJson(const Patient& p);
json toJson(const Doctor& d);
json toJson(const Appointment& a);
json toJson(const Consultation& c);
json toJson(const User& u);
json toJson(const AuditEntry& e);
}  // namespace hms
