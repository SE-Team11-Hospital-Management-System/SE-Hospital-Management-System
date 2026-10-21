// Member 4 (SF-4) - TC-DATA-01 part c (full restart), TC-SEC-05 (HMS-F-060, NF-011, SEC-006)
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
struct DataFixture : ::testing::Test {
    std::string root = hms_test::freshDir("data");
    std::string dir = root + "/data";
    void TearDown() override { std::error_code ec; fs::permissions(root, fs::perms::owner_all, ec); fs::remove_all(root, ec); }
};
}  // namespace

// TC-DATA-01 (c): every service reloads its data at start-up; a failed save shows E041 and changes nothing
TEST_F(DataFixture, TC_DATA_01_FullRestartAndFailedSave) {
    {
        AppContext ctx(dir, root + "/backup");
        ASSERT_TRUE(loadAllData(ctx).ok());
        ASSERT_TRUE(ctx.auth.bootstrapAdmin("admin01", "Admin1234").ok());
        auto admin = ctx.auth.login("admin01", "Admin1234").value();
        auto docUser = ctx.auth.createUser(admin, "doc01", "Doc12345", Role::Doctor).value();
        ASSERT_TRUE(ctx.patients.registerPatient(admin, {"Anita Rao", "1990-05-14", "F", "9876543210", "MG Road", "B+"}).ok());
        auto d = ctx.doctors.addDoctor(admin, {"Meera Iyer", "Cardiology", "Cardiac", docUser}).value();
        ASSERT_TRUE(ctx.doctors.setAvailability(admin, d, {{1, "09:00", "12:00"}, {3, "09:00", "12:00"}}).ok());

        fs::create_directory(dir + "/patients.dat.tmp");                  // next patient save will fail
        auto failed = ctx.patients.registerPatient(admin, {"Anil Kumar", "1985-01-01", "M", "9123456780", "Jayanagar", "O+"});
        ASSERT_FALSE(failed.ok());
        EXPECT_EQ(failed.error().code, ErrorCode::E041);
        EXPECT_EQ(ctx.patientRepo.size(), 1u);                            // nothing changed in memory either
        fs::remove(dir + "/patients.dat.tmp");
    }
    AppContext again(dir, root + "/backup");                              // "restart"
    auto rep = loadAllData(again);
    ASSERT_TRUE(rep.ok());
    EXPECT_EQ(rep.value().patients, 1u);
    EXPECT_EQ(rep.value().doctors, 1u);
    EXPECT_TRUE(again.auth.login("doc01", "Doc12345").ok());
    EXPECT_EQ(again.doctors.get("D001")->windows.size(), 2u);
}

#ifndef _WIN32
// TC-SEC-05: data folder 700, data files 600 (Linux / macOS)
TEST_F(DataFixture, TC_SEC_05_OwnerOnlyPermissions) {
    AppContext ctx(dir, root + "/backup");
    ASSERT_TRUE(loadAllData(ctx).ok());
    ASSERT_TRUE(ctx.auth.bootstrapAdmin("admin01", "Admin1234").ok());
    EXPECT_EQ(fs::status(dir).permissions() & fs::perms::all, fs::perms::owner_all);
    EXPECT_EQ(fs::status(dir + "/users.dat").permissions() & fs::perms::all, fs::perms::owner_read | fs::perms::owner_write);
}
#endif
