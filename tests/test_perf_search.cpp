// Member 2 (SF-2) - TC-PERF-01: 100 timed searches on 10,000 patients, 95% within 2 seconds, with the full
// capacity of 10,000 patients, 100 doctors and 50,000 appointments loaded (HMS-NF-001, HMS-NF-004)
#include "app/bootstrap.h"
#include "common/codec.h"
#include "data/file_store.h"
#include "test_helpers.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <gtest/gtest.h>
#include <iostream>

using namespace hms;

namespace {
void writeCapacityData(const std::string& dir) {
    std::vector<std::string> pats, docs, apps;
    char id[16], slot[32];
    const char* names[] = {"Anita Rao", "Anil Kumar", "Priya Sharma", "Rahul Verma", "Sneha Iyer"};
    for (int i = 1; i <= 10000; ++i) {
        std::snprintf(id, sizeof id, "P%06d", i);
        pats.push_back(codec::join({id, std::string(names[i % 5]) + " " + std::to_string(i), "1990-01-01", "F",
                                    std::to_string(9000000000LL + i), "Some Address", "B+", "1", "2026-10-19"}));
    }
    for (int i = 1; i <= 100; ++i) {
        std::snprintf(id, sizeof id, "D%03d", i);
        docs.push_back(codec::join({id, "Doctor Name", "General", "OPD", "U" + std::to_string(90000 + i), "1@09:00-17:00"}));
    }
    for (int i = 1; i <= 50000; ++i) {
        std::snprintf(id, sizeof id, "A%08d", i);
        std::snprintf(slot, sizeof slot, "2026-%02d-%02d %02d:%s", 1 + (i / 5000) % 12, 1 + (i / 200) % 28, 8 + (i / 100) % 12, (i % 2) ? "00" : "30");
        char d[8]; std::snprintf(d, sizeof d, "D%03d", (i % 100) + 1);
        apps.push_back(codec::join({id, "P000001", d, slot, "U00002", "0"}));
    }
    if (!FileStore::ensureDirectory(dir).ok() || !FileStore::atomicWrite(dir + "/patients.dat", pats).ok() ||
        !FileStore::atomicWrite(dir + "/doctors.dat", docs).ok() || !FileStore::atomicWrite(dir + "/appointments.dat", apps).ok())
        throw std::runtime_error("could not write test data");
}
}  // namespace

TEST(Performance, TC_PERF_01_SearchAtFullCapacity) {
    const std::string root = hms_test::freshDir("perfsearch");
    writeCapacityData(root + "/data");
    AppContext ctx(root + "/data", root + "/backup", root + "/reports");
    auto loaded = loadAllData(ctx);
    ASSERT_TRUE(loaded.ok());
    EXPECT_EQ(loaded.value().patients, 10000u);
    EXPECT_EQ(loaded.value().doctors, 100u);
    EXPECT_EQ(loaded.value().appointments, 50000u);
    EXPECT_LT(loaded.value().seconds, 5.0);                                  // HMS-NF-003 still met at capacity

    const Session rec = ctx.sessions.create("U00002", Role::Receptionist);   // a real, valid session
    std::vector<double> ms;
    for (int i = 0; i < 100; ++i) {
        const SearchKey key = static_cast<SearchKey>(i % 3);
        char id[16]; std::snprintf(id, sizeof id, "P%06d", 1 + i * 97);
        const std::string term = key == SearchKey::Id ? std::string(id) : key == SearchKey::Name ? std::string("sharma") : std::to_string(9000000000LL + 1 + i * 97);
        const auto t0 = std::chrono::steady_clock::now();
        auto r = ctx.patients.search(rec, key, term);
        ms.push_back(std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count());
        ASSERT_TRUE(r.ok());
        ASSERT_FALSE(r.value().items.empty());
    }
    std::sort(ms.begin(), ms.end());
    std::cout << "[ PERF ] 100 searches: median " << ms[49] << " ms, 95th percentile " << ms[94] << " ms\n";
    EXPECT_LT(ms[94], 2000.0);                                               // at least 95 of 100 within 2 s
    std::filesystem::remove_all(root);
}
