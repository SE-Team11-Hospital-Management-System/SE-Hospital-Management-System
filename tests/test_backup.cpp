// Member 4 (SF-4) - TC-DATA-02 backup/restore, TC-PERF-02 start-up time (HMS-F-061, NF-003)
#include "app/bootstrap.h"
#include "common/codec.h"
#include "data/file_store.h"
#include "security/access_control.h"
#include "test_helpers.h"
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

namespace fs = std::filesystem;
using namespace hms;
using hms_test::session;

namespace {
struct BackupFixture : ::testing::Test {
    std::string root = hms_test::freshDir("data");
    std::string dir = root + "/data";
    void TearDown() override { std::error_code ec; fs::permissions(root, fs::perms::owner_all, ec); fs::remove_all(root, ec); }
};
}  // namespace

// TC-DATA-02: backup folder YYYYMMDD_HHMMSS, restore only after confirmation, Administrator only
TEST_F(BackupFixture, TC_DATA_02_BackupAndRestore) {
    AppContext ctx(dir, root + "/backup");
    ASSERT_TRUE(loadAllData(ctx).ok());
    ASSERT_TRUE(ctx.auth.bootstrapAdmin("admin01", "Admin1234").ok());
    auto admin = ctx.auth.login("admin01", "Admin1234").value();
    ASSERT_TRUE(ctx.patients.registerPatient(admin, {"Anita Rao", "1990-05-14", "F", "9876543210", "MG Road", "B+"}).ok());

    auto b = ctx.backup.createBackup(admin);
    ASSERT_TRUE(b.ok());
    EXPECT_EQ(b.value().size(), 15u);
    EXPECT_EQ(b.value()[8], '_');
    EXPECT_TRUE(fs::exists(root + "/backup/" + b.value() + "/patients.dat"));

    ASSERT_TRUE(ctx.patients.registerPatient(admin, {"Anil Kumar", "1985-01-01", "M", "9123456780", "Jayanagar", "O+"}).ok());
    EXPECT_EQ(ctx.backup.restore(admin, b.value(), false).error().code, ErrorCode::E010);   // no confirmation
    EXPECT_EQ(ctx.backup.restore(session(Role::Receptionist), b.value(), true).error().code, ErrorCode::E004);
    EXPECT_EQ(ctx.backup.createBackup(session(Role::Receptionist)).error().code, ErrorCode::E004);
    ASSERT_TRUE(ctx.backup.restore(admin, b.value(), true).ok());
    ASSERT_TRUE(loadAllData(ctx).ok());
    EXPECT_EQ(ctx.patientRepo.size(), 1u);
    EXPECT_EQ(ctx.backup.listBackups(admin).value().front(), b.value());
}

// TC-PERF-02: start-up with 10,000 patients, 100 doctors, 50,000 appointments in under 5 seconds (HMS-NF-003)
TEST_F(BackupFixture, TC_PERF_02_StartupUnder5Seconds) {
    ASSERT_TRUE(FileStore::ensureDirectory(dir).ok());
    std::vector<std::string> pats, docs, apps;
    char id[16];
    for (int i = 1; i <= 10000; ++i) {
        std::snprintf(id, sizeof id, "P%06d", i);
        pats.push_back(codec::join({id, "Patient Name", "1990-01-01", "F", "9876543210", "Some Address", "B+", "1", "2026-10-19"}));
    }
    for (int i = 1; i <= 100; ++i) {
        std::snprintf(id, sizeof id, "D%03d", i);
        docs.push_back(codec::join({id, "Doctor Name", "General", "OPD", "U" + std::to_string(90000 + i), "1@09:00-17:00;2@09:00-17:00"}));
    }
    for (int i = 1; i <= 50000; ++i) {
        char a[16], d[8], slot[20];
        std::snprintf(a, sizeof a, "A%08d", i);
        std::snprintf(d, sizeof d, "D%03d", (i % 100) + 1);
        std::snprintf(slot, sizeof slot, "2026-%02d-%02d %02d:%s", 1 + (i / 5000) % 12, 1 + (i / 200) % 28, 8 + (i / 100) % 12, (i % 2) ? "00" : "30");
        apps.push_back(codec::join({a, "P000001", d, slot, "U00002", "0"}));
    }
    ASSERT_TRUE(FileStore::atomicWrite(dir + "/patients.dat", pats).ok());
    ASSERT_TRUE(FileStore::atomicWrite(dir + "/doctors.dat", docs).ok());
    ASSERT_TRUE(FileStore::atomicWrite(dir + "/appointments.dat", apps).ok());

    AppContext ctx(dir, root + "/backup");
    auto rep = loadAllData(ctx);
    ASSERT_TRUE(rep.ok());
    EXPECT_EQ(rep.value().patients, 10000u);
    EXPECT_EQ(rep.value().doctors, 100u);
    EXPECT_EQ(rep.value().appointments, 50000u);
    EXPECT_LT(rep.value().seconds, 5.0);
    std::cout << "[ PERF ] start-up load took " << rep.value().seconds << " s\n";
}

