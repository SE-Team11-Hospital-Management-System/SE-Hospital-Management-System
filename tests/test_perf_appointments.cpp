// Member 3 (SF-3) - TC-PERF-02 (operations part): book / cancel / reschedule with 50,000 stored appointments,
// at least 95 of 100 operations within 1 second including the disk write (HMS-NF-002)
#include "common/codec.h"
#include "data/file_store.h"
#include "services/appointment_service.h"
#include "test_helpers.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <gtest/gtest.h>
#include <iostream>
#include <stdexcept>

using namespace hms;
using hms_test::session;

// Sanitizer builds (CI memory-safety job) run 3-5 times slower, so the 1-second rule is only checked in normal builds.
#if defined(__SANITIZE_ADDRESS__)
#define HMS_SLOW_BUILD 1
#elif defined(__has_feature)
#if __has_feature(address_sanitizer)
#define HMS_SLOW_BUILD 1
#endif
#endif
#ifdef HMS_SLOW_BUILD
constexpr double kLimitMs = 3000.0;
#else
constexpr double kLimitMs = 1000.0;
#endif

namespace {
// 100 doctors available every day, 50,000 appointments in 2026 (500 per doctor).
void writeFullLoad(const std::string& dir) {
    std::vector<std::string> docs, apps;
    char id[16], slot[32];
    for (int i = 1; i <= 100; ++i) {
        std::snprintf(id, sizeof id, "D%03d", i);
        docs.push_back(codec::join({id, "Doctor Name", "General", "OPD", "U" + std::to_string(90000 + i), "0@08:00-20:00;1@08:00-20:00;2@08:00-20:00;3@08:00-20:00;4@08:00-20:00;5@08:00-20:00;6@08:00-20:00"}));
    }
    for (int i = 1; i <= 50000; ++i) {                                         // 2026, 500 per doctor
        std::snprintf(id, sizeof id, "A%08d", i);
        std::snprintf(slot, sizeof slot, "2026-%02d-%02d %02d:%s", 1 + (i / 5000) % 12, 1 + (i / 200) % 28, 8 + (i / 100) % 12, (i % 2) ? "00" : "30");
        char d[8]; std::snprintf(d, sizeof d, "D%03d", (i % 100) + 1);
        apps.push_back(codec::join({id, "P000001", d, slot, "U00002", "0"}));
    }
    if (!FileStore::atomicWrite(dir + "/doctors.dat", docs).ok() || !FileStore::atomicWrite(dir + "/appointments.dat", apps).ok())
        throw std::runtime_error("could not write test data");

}
}  // namespace

TEST(Performance, TC_PERF_02_BookCancelRescheduleAtFullLoad) {
    const std::string dir = hms_test::freshDir("perfappt");
    writeFullLoad(dir);
    char slot[32];
    AuditService audit(dir + "/audit.log");
    PatientRepository patients;
    patients.put(Patient{"P000001", "Anita Rao", "1990-05-14", "F", "9876543210", "MG Road", "B+", true, "2026-10-19"});
    DoctorService doctors(audit, dir, [](const std::string&) { return true; });
    AppointmentService appts(patients, doctors, audit, dir, [] { return std::string("2026-12-31 08:00"); });
    ASSERT_TRUE(doctors.load().ok());
    ASSERT_TRUE(appts.load().ok());
    ASSERT_EQ(appts.all().size(), 50000u);

    const Session rec = session(Role::Receptionist, "U00002");
    std::vector<double> ms;
    auto timed = [&ms](auto&& op) {
        const auto t0 = std::chrono::steady_clock::now();
        const bool ok = op();
        ms.push_back(std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count());
        return ok;
    };
    std::vector<std::string> booked;
    for (int i = 0; i < 40; ++i) {                                            // 40 bookings in 2027
        std::snprintf(slot, sizeof slot, "2027-01-%02d %02d:00", 1 + i % 28, 9 + i / 28);
        ASSERT_TRUE(timed([&] { auto r = appts.book(rec, "P000001", "D001", slot); if (r.ok()) booked.push_back(r.value()); return r.ok(); }));
    }
    for (int i = 0; i < 30; ++i) {                                            // 30 reschedules
        std::snprintf(slot, sizeof slot, "2027-02-%02d %02d:30", 1 + i % 28, 9 + i / 28);
        ASSERT_TRUE(timed([&] { return appts.reschedule(rec, booked[static_cast<size_t>(i)], "D002", slot).ok(); }));
    }
    for (int i = 0; i < 30; ++i)                                              // 30 cancellations
        ASSERT_TRUE(timed([&] { return appts.cancel(rec, booked[static_cast<size_t>(10 + i)]).ok(); }));

    ASSERT_EQ(ms.size(), 100u);
    std::sort(ms.begin(), ms.end());
    std::cout << "[ PERF ] 100 operations: median " << ms[49] << " ms, 95th percentile " << ms[94] << " ms\n";
    EXPECT_LT(ms[94], kLimitMs);                                              // at least 95 of 100 within 1 s
    std::filesystem::remove_all(dir);
}
