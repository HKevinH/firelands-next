#include <gtest/gtest.h>

#include "BoundingIntervalHierarchy.h"
#include "Vec3.h"

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <vector>

using namespace Firelands::VMap;

// ─── helpers ─────────────────────────────────────────────────────────────────

struct Box {
    AaBox3 bound;
};

static auto MakeGetBounds() {
    return [](Box const& b, AaBox3& out) { out = b.bound; };
}

static Box MakeBox(float lx, float ly, float lz,
                   float hx, float hy, float hz) {
    Box b;
    b.bound.lo = {lx, ly, lz};
    b.bound.hi = {hx, hy, hz};
    return b;
}

// ─── BihRoundTrip ─────────────────────────────────────────────────────────────

TEST(BihTests, RoundTripEmpty) {
    BIH bih;
    std::vector<Box> prims;
    auto gb = MakeGetBounds();
    bih.Build(prims, gb);
    EXPECT_EQ(bih.PrimCount(), 0u);

    auto path = std::filesystem::temp_directory_path() / "bih_empty_roundtrip.bin";
    {
        FILE* wf = std::fopen(path.string().c_str(), "wb");
        ASSERT_NE(wf, nullptr);
        EXPECT_TRUE(bih.WriteToFile(wf));
        std::fclose(wf);
    }
    {
        BIH loaded;
        FILE* rf = std::fopen(path.string().c_str(), "rb");
        ASSERT_NE(rf, nullptr);
        EXPECT_TRUE(loaded.ReadFromFile(rf));
        std::fclose(rf);
        EXPECT_EQ(loaded.PrimCount(), 0u);
    }
    std::filesystem::remove(path);
}

TEST(BihTests, RoundTripSinglePrim) {
    BIH bih;
    std::vector<Box> prims = { MakeBox(0,0,0, 1,1,1) };
    auto gb = MakeGetBounds();
    bih.Build(prims, gb);
    EXPECT_EQ(bih.PrimCount(), 1u);

    auto path = std::filesystem::temp_directory_path() / "bih_single_roundtrip.bin";
    {
        FILE* wf = std::fopen(path.string().c_str(), "wb");
        ASSERT_NE(wf, nullptr);
        EXPECT_TRUE(bih.WriteToFile(wf));
        std::fclose(wf);
    }
    {
        BIH loaded;
        FILE* rf = std::fopen(path.string().c_str(), "rb");
        ASSERT_NE(rf, nullptr);
        EXPECT_TRUE(loaded.ReadFromFile(rf));
        std::fclose(rf);
        EXPECT_EQ(loaded.PrimCount(), 1u);
    }
    std::filesystem::remove(path);
}

TEST(BihTests, RoundTripManyPrims) {
    BIH bih;
    std::vector<Box> prims;
    for (int i = 0; i < 100; ++i) {
        float fi = static_cast<float>(i);
        prims.push_back(MakeBox(fi, fi, fi, fi+1, fi+1, fi+1));
    }
    auto gb = MakeGetBounds();
    bih.Build(prims, gb);
    EXPECT_EQ(bih.PrimCount(), 100u);

    auto path = std::filesystem::temp_directory_path() / "bih_many_roundtrip.bin";
    {
        FILE* wf = std::fopen(path.string().c_str(), "wb");
        ASSERT_NE(wf, nullptr);
        EXPECT_TRUE(bih.WriteToFile(wf));
        std::fclose(wf);
    }
    {
        BIH loaded;
        FILE* rf = std::fopen(path.string().c_str(), "rb");
        ASSERT_NE(rf, nullptr);
        EXPECT_TRUE(loaded.ReadFromFile(rf));
        std::fclose(rf);
        EXPECT_EQ(loaded.PrimCount(), 100u);
    }
    std::filesystem::remove(path);
}

// ─── BihPointQuery ────────────────────────────────────────────────────────────
// Build a tree over boxes, then verify IntersectPoint hits the correct one.

TEST(BihTests, PointQueryHits) {
    std::vector<Box> prims = {
        MakeBox(0,0,0,  10,10,10),
        MakeBox(20,0,0, 30,10,10),
        MakeBox(40,0,0, 50,10,10),
    };
    BIH bih;
    auto gb = MakeGetBounds();
    bih.Build(prims, gb, 1);

    std::vector<uint32_t> hits;
    auto cb = [&](Vec3 const&, uint32_t idx) { hits.push_back(idx); };

    bih.IntersectPoint({5, 5, 5}, cb);
    ASSERT_EQ(hits.size(), 1u);
    EXPECT_EQ(hits[0], 0u);

    hits.clear();
    bih.IntersectPoint({25, 5, 5}, cb);
    ASSERT_EQ(hits.size(), 1u);
    EXPECT_EQ(hits[0], 1u);
}

TEST(BihTests, PointQueryMiss) {
    std::vector<Box> prims = { MakeBox(0,0,0, 1,1,1) };
    BIH bih;
    auto gb = MakeGetBounds();
    bih.Build(prims, gb);

    int hitCount = 0;
    auto cb = [&](Vec3 const&, uint32_t) { ++hitCount; };
    bih.IntersectPoint({100, 100, 100}, cb);
    EXPECT_EQ(hitCount, 0);
}
