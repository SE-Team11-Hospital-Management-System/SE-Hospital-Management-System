// Member 4 (SF-4) - TC-DATA-01 parts a/b: atomic write, write failure (HMS-NF-005, NF-011)
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
struct FileStoreFixture : ::testing::Test {
    std::string root = hms_test::freshDir("data");
    std::string dir = root + "/data";
    void TearDown() override { std::error_code ec; fs::permissions(root, fs::perms::owner_all, ec); fs::remove_all(root, ec); }
};
}  // namespace

// TC-DATA-01 (a): atomic write replaces the file completely and leaves no temp file
TEST_F(FileStoreFixture, TC_DATA_01_AtomicWriteAndLoad) {
    ASSERT_TRUE(FileStore::ensureDirectory(dir).ok());
    const std::string f = dir + "/x.dat";
    ASSERT_TRUE(FileStore::atomicWrite(f, {"a", "b"}).ok());
    ASSERT_TRUE(FileStore::atomicWrite(f, {"c"}).ok());
    auto r = FileStore::load(f);
    ASSERT_TRUE(r.ok());
    EXPECT_EQ(r.value(), std::vector<std::string>{"c"});
    EXPECT_FALSE(fs::exists(f + ".tmp"));
    EXPECT_TRUE(FileStore::load(dir + "/missing.dat").value().empty());   // first run = empty, not an error
}

// TC-DATA-01 (b): a failed write returns E041 and the old file is unchanged (HMS-NF-011, NF-005)
TEST_F(FileStoreFixture, TC_DATA_01_WriteFailureKeepsOldData) {
    ASSERT_TRUE(FileStore::ensureDirectory(dir).ok());
    const std::string f = dir + "/x.dat";
    ASSERT_TRUE(FileStore::atomicWrite(f, {"old"}).ok());
    fs::create_directory(f + ".tmp");                                     // simulate a disk problem: temp file cannot be created
    auto w = FileStore::atomicWrite(f, {"new"});
    ASSERT_FALSE(w.ok());
    EXPECT_EQ(w.error().code, ErrorCode::E041);
    EXPECT_EQ(FileStore::load(f).value(), std::vector<std::string>{"old"});
}

