// Member 2 (SF-2) - TC-SEC-06 tamper detection by checksum and restore offer (HMS-SEC-007)
#include "app/recovery.h"
#include "data/file_store.h"
#include "test_helpers.h"
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

using namespace hms;
namespace fs = std::filesystem;

namespace {
void flipOneByte(const std::string& path) {
    std::fstream f(path, std::ios::in | std::ios::out | std::ios::binary);
    f.seekg(3); char c = 0; f.get(c);
    f.seekp(3); f.put(c == 'X' ? 'Y' : 'X');
}
}  // namespace

TEST(Integrity, ChecksumLineWrittenAndChecked) {
    const std::string dir = hms_test::freshDir("sum");
    ASSERT_TRUE(FileStore::atomicWrite(dir + "/x.dat", {"a|b", "c"}).ok());
    EXPECT_EQ(FileStore::load(dir + "/x.dat").value(), (std::vector<std::string>{"a|b", "c"}));
    flipOneByte(dir + "/x.dat");
    EXPECT_EQ(FileStore::load(dir + "/x.dat").error().code, ErrorCode::E040);
    std::ofstream(dir + "/old.dat") << "no checksum line\n";                // files without a checksum are refused too
    EXPECT_EQ(FileStore::load(dir + "/old.dat").error().code, ErrorCode::E040);
    fs::remove_all(dir);
}

// TC-SEC-06: a damaged patients.dat is not loaded; E040, audit entry, Administrator restores the latest backup
TEST(Integrity, TC_SEC_06_TamperDetectedAndRestored) {
    hms_test::World w;
    ASSERT_TRUE(w.ctx->backup.createBackup(w.admin).ok());
    flipOneByte(w.dir + "/patients.dat");
    AppContext fresh(w.dir, w.root + "/backup", w.root + "/reports");      // "start HMS again"
    auto loaded = loadAllData(fresh);
    ASSERT_FALSE(loaded.ok());
    EXPECT_EQ(loaded.error().code, ErrorCode::E040);

    EXPECT_EQ(restoreLatestBackup(fresh, "rec01", "Rec@1234").error().code, ErrorCode::E004);   // not an Administrator
    EXPECT_EQ(restoreLatestBackup(fresh, "admin01", "Wrong9999").error().code, ErrorCode::E001);
    auto restored = restoreLatestBackup(fresh, "admin01", "Admin1234");
    ASSERT_TRUE(restored.ok()) << restored.error().message;
    EXPECT_TRUE(fresh.patients.get(w.p1).has_value());
}
