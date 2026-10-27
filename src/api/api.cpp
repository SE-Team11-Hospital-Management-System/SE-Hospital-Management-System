#include "api/api.h"
#include <httplib.h>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace hms {

// ---------------------------------------------------------------- Call
std::string Call::param(const std::string& name) const {
    auto it = req.path_params.find(name);
    return it == req.path_params.end() ? "" : it->second;
}
std::string Call::query(const std::string& name) const { return req.has_param(name) ? req.get_param_value(name) : ""; }
std::string Call::str(const std::string& field) const {
    return body.is_object() && body.contains(field) && body[field].is_string() ? body[field].get<std::string>() : "";
}
void Call::ok(const json& data, int status) const {
    res.status = status;
    res.set_content(data.dump(), "application/json");
}
void Call::fail(const Error& e) const {
    res.status = Api::httpStatus(e.code);
    res.set_content(json{{"error", {{"code", codeName(e.code)}, {"message", e.message}, {"details", e.details}}}}.dump(), "application/json");
}
void Call::text(const std::string& data, const std::string& contentType, const std::string& fileName) const {
    res.status = 200;
    if (!fileName.empty()) res.set_header("Content-Disposition", "attachment; filename=\"" + fileName + "\"");
    res.set_content(data, contentType);
}

// ---------------------------------------------------------------- Api
namespace {
// HMS-SEC-009: only pages served by this computer may call the API. A request whose Host or Origin names another
// site is refused, which blocks DNS-rebinding and cross-site requests from other websites open in the browser.
bool fromThisComputer(const httplib::Request& req) {
    auto localName = [](std::string v, bool isOrigin) {
        if (isOrigin) {
            if (v.rfind("http://", 0) != 0) return false;
            v = v.substr(7);
        }
        const auto colon = v.rfind(':');
        const std::string host = colon == std::string::npos ? v : v.substr(0, colon);
        return host == "localhost" || host == "127.0.0.1";
    };
    const std::string host = req.get_header_value("Host");
    if (!host.empty() && !localName(host, false)) return false;
    const std::string origin = req.get_header_value("Origin");
    return origin.empty() || localName(origin, true);
}
}  // namespace

int Api::httpStatus(ErrorCode c) {
    switch (c) {
        case ErrorCode::OK: return 200;
        case ErrorCode::E001: case ErrorCode::E003: return 401;
        case ErrorCode::E002: return 423;
        case ErrorCode::E004: return 403;
        case ErrorCode::E010: return 400;
        case ErrorCode::E020: case ErrorCode::E021: case ErrorCode::E022: case ErrorCode::E023: case ErrorCode::E024: return 404;
        case ErrorCode::E011: case ErrorCode::E025: case ErrorCode::E030: case ErrorCode::E031: case ErrorCode::E032:
        case ErrorCode::E043: return 409;
        case ErrorCode::E040: return 503;
        default: return 500;
    }
}

Api::Api(AppContext& ctx, std::string webRoot) : ctx_(ctx), webRoot_(std::move(webRoot)), server_(std::make_unique<httplib::Server>()) {
    std::error_code ec;
    if (std::filesystem::is_directory(webRoot_, ec)) server_->set_mount_point("/", webRoot_);   // the built website
    // Any other non-API address is a page of the website: send index.html and let React show it.
    server_->Get(R"(/(?!api/).*)", [this](const httplib::Request&, httplib::Response& res) {
        std::ifstream in(webRoot_ + "/index.html", std::ios::binary);
        if (!in) { res.status = 404; res.set_content("Website not built yet. Run:  python sprint.py run", "text/plain"); return; }
        std::stringstream ss; ss << in.rdbuf();
        res.set_content(ss.str(), "text/html");
    });
    server_->set_payload_max_length(1024 * 1024);                              // 1 MB per request is plenty
}

Api::~Api() = default;

void Api::add(const std::string& method, const std::string& path, Handler h, bool auth) {
    auto wrapped = [this, path, h, auth](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> guard(lock_);                              // one request at a time
        res.set_header("Cache-Control", "no-store");
        res.set_header("X-Content-Type-Options", "nosniff");
        Call c{req, res, Session{}, json::object()};
        try {
            if (!fromThisComputer(req)) { c.fail(makeError(ErrorCode::E004, "Request refused: it did not come from this computer.")); return; }
            if (recovery_ && !recoveryAllowed_.count(path)) { c.fail(damage_); return; }
            if (!req.body.empty()) {
                c.body = json::parse(req.body, nullptr, false);
                if (c.body.is_discarded()) { c.fail(makeError(ErrorCode::E010, "Request: the data sent is not valid JSON.")); return; }
            }
            if (auth) {
                const std::string h = req.get_header_value("Authorization");
                const std::string token = h.rfind("Bearer ", 0) == 0 ? h.substr(7) : "";
                auto s = ctx_.sessions.validate(token);                         // HMS-F-005: E003 after 10 idle minutes
                if (!s.ok()) { c.fail(s.error()); return; }
                c.session = s.value();
            }
            h(c);
        } catch (const std::exception&) {
            c.fail(makeError(ErrorCode::E099, "Unexpected error. Nothing was changed."));
        }
    };
    if (method == "GET") server_->Get(path, wrapped);
    else if (method == "POST") server_->Post(path, wrapped);
    else server_->Put(path, wrapped);
}

void Api::get(const std::string& path, Handler h, bool auth) { add("GET", path, std::move(h), auth); }
void Api::post(const std::string& path, Handler h, bool auth) { add("POST", path, std::move(h), auth); }
void Api::put(const std::string& path, Handler h, bool auth) { add("PUT", path, std::move(h), auth); }

void Api::enterRecovery(const Error& damage) {
    recovery_ = true;
    damage_ = damage;
    const std::string file = std::filesystem::path(damage.details).filename().string();
    ctx_.audit.log("-", "INTEGRITY_FAIL", file.empty() ? "-" : file, false);    // HMS-SEC-007
}

int Api::bind(const std::string& host, int port) {
    if (port == 0) return server_->bind_to_any_port(host);
    return server_->bind_to_port(host, port) ? port : -1;
}
bool Api::serve() { return server_->listen_after_bind(); }
void Api::stop() { server_->stop(); }

// ---------------------------------------------------------------- JSON shapes
json toJson(const Patient& p) {
    return {{"patientId", p.patientId}, {"fullName", p.fullName}, {"dob", p.dob}, {"gender", p.gender}, {"phone", p.phone},
            {"address", p.address}, {"bloodGroup", p.bloodGroup}, {"active", p.active}, {"registeredOn", p.registeredOn}};
}
json toJson(const Doctor& d) {
    json w = json::array();
    for (const auto& x : d.windows) w.push_back({{"weekday", x.weekday}, {"start", x.start}, {"end", x.end}});
    return {{"doctorId", d.doctorId}, {"fullName", d.fullName}, {"specialization", d.specialization},
            {"department", d.department}, {"userId", d.userId}, {"windows", w}};
}
json toJson(const Appointment& a) {
    return {{"appointmentId", a.appointmentId}, {"patientId", a.patientId}, {"doctorId", a.doctorId},
            {"slotStart", a.slotStart}, {"bookedBy", a.bookedBy}, {"status", toString(a.status)}};
}
json toJson(const Consultation& c) {
    json items = json::array(), adds = json::array();
    for (const auto& i : c.items) items.push_back({{"medicine", i.medicine}, {"dosage", i.dosage}, {"days", i.days}});
    for (const auto& a : c.addenda) adds.push_back({{"timestamp", a.timestamp}, {"authorUserId", a.authorUserId}, {"text", a.text}});
    return {{"consultId", c.consultId}, {"appointmentId", c.appointmentId}, {"patientId", c.patientId}, {"doctorId", c.doctorId},
            {"createdAt", c.createdAt}, {"diagnosis", c.diagnosis}, {"notes", c.notes}, {"items", items}, {"addenda", adds}};
}
json toJson(const User& u) {
    return {{"userId", u.userId}, {"username", u.username}, {"role", roleName(u.role)}, {"active", u.active}};
}
json toJson(const AuditEntry& e) {
    return {{"timestamp", e.timestamp}, {"userId", e.userId}, {"action", e.action}, {"targetId", e.targetId}, {"outcome", e.outcome}};
}
}  // namespace hms
