#include <gtest/gtest.h>

#include "ModelSpawn.h"

#include <cstdio>
#include <filesystem>

using namespace Firelands::VMap;

// ─── helpers ─────────────────────────────────────────────────────────────────

static ModelSpawn MakeSpawn(bool hasBound) {
    ModelSpawn s;
    s.flags  = kModM2 | (hasBound ? kModHasBound : 0);
    s.adtId  = 7;
    s.ID     = 12345;
    s.iPos   = {1.f, 2.f, 3.f};
    s.iRot   = {10.f, 20.f, 30.f};
    s.iScale = 0.75f;
    if (hasBound) {
        s.iBound.lo = {-5.f, -5.f, -5.f};
        s.iBound.hi = { 5.f,  5.f,  5.f};
    }
    s.name = "World/WMO/Dungeon/Example.wmo";
    return s;
}

// ─── ModelSpawnRoundTrip ──────────────────────────────────────────────────────

TEST(ModelSpawnTests, RoundTripNoBound) {
    ModelSpawn orig = MakeSpawn(false);
    auto path = std::filesystem::temp_directory_path() / "spawn_no_bound.bin";
    {
        FILE* wf = std::fopen(path.string().c_str(), "wb");
        ASSERT_NE(wf, nullptr);
        EXPECT_TRUE(ModelSpawn::WriteToFile(wf, orig));
        std::fclose(wf);
    }
    {
        FILE* rf = std::fopen(path.string().c_str(), "rb");
        ASSERT_NE(rf, nullptr);
        ModelSpawn loaded;
        EXPECT_TRUE(ModelSpawn::ReadFromFile(rf, loaded));
        std::fclose(rf);

        EXPECT_EQ(loaded.flags,  orig.flags);
        EXPECT_EQ(loaded.adtId,  orig.adtId);
        EXPECT_EQ(loaded.ID,     orig.ID);
        EXPECT_FLOAT_EQ(loaded.iPos.x, orig.iPos.x);
        EXPECT_FLOAT_EQ(loaded.iPos.y, orig.iPos.y);
        EXPECT_FLOAT_EQ(loaded.iPos.z, orig.iPos.z);
        EXPECT_FLOAT_EQ(loaded.iRot.x, orig.iRot.x);
        EXPECT_FLOAT_EQ(loaded.iScale, orig.iScale);
        EXPECT_EQ(loaded.name,   orig.name);
        EXPECT_FALSE(loaded.HasBound());
    }
    std::filesystem::remove(path);
}

TEST(ModelSpawnTests, RoundTripWithBound) {
    ModelSpawn orig = MakeSpawn(true);
    auto path = std::filesystem::temp_directory_path() / "spawn_with_bound.bin";
    {
        FILE* wf = std::fopen(path.string().c_str(), "wb");
        ASSERT_NE(wf, nullptr);
        EXPECT_TRUE(ModelSpawn::WriteToFile(wf, orig));
        std::fclose(wf);
    }
    {
        FILE* rf = std::fopen(path.string().c_str(), "rb");
        ASSERT_NE(rf, nullptr);
        ModelSpawn loaded;
        EXPECT_TRUE(ModelSpawn::ReadFromFile(rf, loaded));
        std::fclose(rf);

        EXPECT_TRUE(loaded.HasBound());
        EXPECT_FLOAT_EQ(loaded.iBound.lo.x, orig.iBound.lo.x);
        EXPECT_FLOAT_EQ(loaded.iBound.hi.z, orig.iBound.hi.z);
        EXPECT_EQ(loaded.name, orig.name);
    }
    std::filesystem::remove(path);
}

TEST(ModelSpawnTests, EofReturnsfalse) {
    auto path = std::filesystem::temp_directory_path() / "spawn_empty.bin";
    { FILE* wf = std::fopen(path.string().c_str(), "wb"); std::fclose(wf); }
    FILE* rf = std::fopen(path.string().c_str(), "rb");
    ASSERT_NE(rf, nullptr);
    ModelSpawn s;
    EXPECT_FALSE(ModelSpawn::ReadFromFile(rf, s));
    std::fclose(rf);
    std::filesystem::remove(path);
}

TEST(ModelSpawnTests, FlagHelpers) {
    ModelSpawn s;
    s.flags = kModM2 | kModHasBound;
    EXPECT_TRUE(s.IsM2());
    EXPECT_TRUE(s.HasBound());
    EXPECT_FALSE(s.IsParent());

    s.flags = kModParentSpawn;
    EXPECT_FALSE(s.IsM2());
    EXPECT_TRUE(s.IsParent());
}
