// Skeleton tests (TASK-02): the build works and the shared helpers behave.
#include "common/codec.h"
#include "common/datetime.h"
#include "common/types.h"
#include "security/sha256.h"
#include "security/validator.h"
#include <gtest/gtest.h>

using namespace hms;

TEST(Skeleton, Sha256KnownVectors) {
    EXPECT_EQ(sha256Hex(""), "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    EXPECT_EQ(sha256Hex("abc"), "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
}

TEST(Skeleton, ResultCarriesValueOrError) {
    Result<int> ok = 5;
    Result<int> bad = makeError(ErrorCode::E010, "x");
    EXPECT_TRUE(ok.ok()); EXPECT_EQ(ok.value(), 5);
    EXPECT_FALSE(bad.ok()); EXPECT_STREQ(codeName(bad.error().code), "E010");
}

TEST(Skeleton, CodecRoundTripWithPipesAndBackslashes) {
    const std::vector<std::string> f = {"a|b", "c\\d", "", "plain"};
    EXPECT_EQ(codec::split(codec::join(f)), f);
}

TEST(Skeleton, DateTimeHelpers) {
    EXPECT_TRUE(dt::isValid("2026-10-19 09:30"));
    EXPECT_FALSE(dt::isValid("2026-02-30 09:30"));
    EXPECT_EQ(dt::weekday("2026-10-19 09:30"), 1);                       // Monday
    EXPECT_EQ(dt::addMinutes("2026-10-19 23:30", 30), "2026-10-20 00:00");
}

// TC-SEC-02 (part): validator rules used by every screen (HMS-SEC-005, HMS-F-011)
TEST(Skeleton, ValidatorRules) {
    EXPECT_TRUE(Validator::isValidUsername("rec01"));
    EXPECT_FALSE(Validator::isValidUsername("ab"));
    EXPECT_TRUE(Validator::isValidPassword("Rec@1234"));
    EXPECT_FALSE(Validator::isValidPassword("abcdefgh"));
    EXPECT_TRUE(Validator::isValidPhone("9876543210"));
    EXPECT_FALSE(Validator::isValidPhone("98765abcde"));
    EXPECT_FALSE(Validator::isValidLineLength(std::string(5000, 'a')));
    EXPECT_TRUE(Validator::isValidDate("2024-02-29"));
    EXPECT_FALSE(Validator::isValidDate("2023-02-29"));
}
