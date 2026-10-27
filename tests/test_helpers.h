// Shared test helpers: a fresh temporary data folder per test, ready-made sessions, and a small "world"
// (a full AppContext with an Administrator, a Receptionist, two Doctors and two patients).
#pragma once
#include "app/app_context.h"
#include "app/bootstrap.h"
#include "security/session_manager.h"
#include <filesystem>
#include <memory>
#include <random>
#include <stdexcept>
#include <string>

namespace hms_test {
inline std::string freshDir(const std::string& tag) {
    std::random_device rd;
    auto p = std::filesystem::temp_directory_path() / ("hms_test_" + tag + "_" + std::to_string(rd()));
    std::filesystem::remove_all(p);
    std::filesystem::create_directories(p);
    return p.string();
}
inline hms::Session session(hms::Role r, const std::string& userId = "U00001") {
    hms::Session s; s.token = "t-" + userId; s.userId = userId; s.role = r; return s;
}

template <class T> T must(const hms::Result<T>& r) {
    if (!r.ok()) throw std::runtime_error(r.error().message);
    return r.value();
}

struct World {
    std::string root = freshDir("world");
    std::string dir = root + "/data";
    std::unique_ptr<hms::AppContext> ctx;
    hms::Session admin, rec, doc1, doc2;
    std::string d1, d2;                                  // D001 (doc01), D002 (doc02), available every day 08:00-20:00
    std::string p1, p2;                                  // P000001, P000002

    World() {
        using namespace hms;
        ctx = std::make_unique<AppContext>(dir, root + "/backup", root + "/reports");
        must(loadAllData(*ctx));
        must(ctx->auth.bootstrapAdmin("admin01", "Admin1234"));
        admin = must(ctx->auth.login("admin01", "Admin1234"));
        must(ctx->auth.createUser(admin, "rec01", "Rec@1234", Role::Receptionist));
        const auto u3 = must(ctx->auth.createUser(admin, "doc01", "Doc12345", Role::Doctor));
        const auto u4 = must(ctx->auth.createUser(admin, "doc02", "Doc12345", Role::Doctor));
        rec = must(ctx->auth.login("rec01", "Rec@1234"));
        d1 = must(ctx->doctors.addDoctor(admin, {"Meera Iyer", "Cardiology", "Cardiac", u3}));
        d2 = must(ctx->doctors.addDoctor(admin, {"Ravi Shah", "Orthopaedics", "Ortho", u4}));
        std::vector<Window> all;
        for (int day = 0; day < 7; ++day) all.push_back(Window{day, "08:00", "20:00"});
        if (!ctx->doctors.setAvailability(admin, d1, all).ok() || !ctx->doctors.setAvailability(admin, d2, all).ok())
            throw std::runtime_error("availability");
        doc1 = must(ctx->auth.login("doc01", "Doc12345")); doc1.doctorId = d1;
        doc2 = must(ctx->auth.login("doc02", "Doc12345")); doc2.doctorId = d2;
        p1 = must(ctx->patients.registerPatient(rec, {"Anita Rao", "1990-05-14", "F", "9876543210", "12 MG Road", "B+"}));
        p2 = must(ctx->patients.registerPatient(rec, {"Anil Kumar", "1985-01-01", "M", "9123456780", "Jayanagar", "O+"}));
    }
    ~World() { ctx.reset(); std::error_code ec; std::filesystem::remove_all(root, ec); }
    World(const World&) = delete;
    World& operator=(const World&) = delete;
};
}  // namespace hms_test
