#include <gtest/gtest.h>

#include "MapFileWriter.h"

#include <bitset>
#include <cstring>
#include <filesystem>
#include <fstream>

namespace FVE = Firelands::VMap::MapExtractor;

// ─── helpers ──────────────────────────────────────────────────────────────────

// Reads the binary file at `path` into `buf`. Returns true on success.
static bool ReadFile(const std::filesystem::path& path, std::vector<char>& buf)
{
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) return false;
    buf.resize(static_cast<size_t>(f.tellg()));
    f.seekg(0);
    f.read(buf.data(), static_cast<std::streamsize>(buf.size()));
    return f.good();
}

template<typename T>
static T ReadAt(const std::vector<char>& buf, size_t offset)
{
    T val{};
    std::memcpy(&val, buf.data() + offset, sizeof(T));
    return val;
}

static constexpr uint32_t kBuild = 15595;

// Offsets that are always the same when fullAreaData == false (single area ID).
static constexpr size_t kAreaHeaderOffset   = sizeof(FVE::map_fileheader);          // 44
static constexpr size_t kHeightHeaderOffset = kAreaHeaderOffset
                                            + sizeof(FVE::map_areaHeader);           // 52

// ─── FlatMapWritesValidHeader ─────────────────────────────────────────────────

TEST(MapFileWriterTests, FlatMapWritesValidHeader)
{
    FVE::AdtGridData grid{};
    for (int y = 0; y <= FVE::kAdtGridSize; ++y)
        for (int x = 0; x <= FVE::kAdtGridSize; ++x)
            grid.V9[y][x] = 100.f;
    for (int y = 0; y < FVE::kAdtGridSize; ++y)
        for (int x = 0; x < FVE::kAdtGridSize; ++x)
            grid.V8[y][x] = 100.f;

    auto path = std::filesystem::temp_directory_path() / "flat_map_header.map";
    ASSERT_TRUE(FVE::MapFileWriter::Write(grid, path, kBuild));

    std::vector<char> buf;
    ASSERT_TRUE(ReadFile(path, buf));
    std::filesystem::remove(path);

    FVE::map_fileheader hdr{};
    std::memcpy(&hdr, buf.data(), sizeof(hdr));

    EXPECT_EQ(hdr.mapMagic,     FVE::kMapMagic);
    EXPECT_EQ(hdr.versionMagic, FVE::kMapVersionMagic);
    EXPECT_EQ(hdr.buildMagic,   kBuild);
    EXPECT_EQ(hdr.areaMapOffset, static_cast<uint32_t>(sizeof(FVE::map_fileheader)));

    FVE::map_heightHeader heightHdr{};
    std::memcpy(&heightHdr, buf.data() + kHeightHeaderOffset, sizeof(heightHdr));
    EXPECT_TRUE(heightHdr.flags & FVE::kMapHeightNoHeight);
}

// ─── VaryingHeightPackedAsInt8 ────────────────────────────────────────────────

TEST(MapFileWriterTests, VaryingHeightPackedAsInt8)
{
    // diff = 1.5f  <  kFloatToInt8Limit (2.0f)  →  int8 packing
    FVE::AdtGridData grid{};
    grid.V9[0][0] = 1.5f;   // all others remain 0.0f

    auto path = std::filesystem::temp_directory_path() / "height_int8.map";
    ASSERT_TRUE(FVE::MapFileWriter::Write(grid, path, kBuild));

    std::vector<char> buf;
    ASSERT_TRUE(ReadFile(path, buf));
    std::filesystem::remove(path);

    FVE::map_heightHeader heightHdr{};
    std::memcpy(&heightHdr, buf.data() + kHeightHeaderOffset, sizeof(heightHdr));

    EXPECT_TRUE(heightHdr.flags & FVE::kMapHeightAsInt8);
}

// ─── VaryingHeightPackedAsInt16 ───────────────────────────────────────────────

TEST(MapFileWriterTests, VaryingHeightPackedAsInt16)
{
    // diff = 500f  →  kFloatToInt8Limit (2.0) < 500 < kFloatToInt16Limit (2048)
    FVE::AdtGridData grid{};
    grid.V9[0][0] = 500.f;

    auto path = std::filesystem::temp_directory_path() / "height_int16.map";
    ASSERT_TRUE(FVE::MapFileWriter::Write(grid, path, kBuild));

    std::vector<char> buf;
    ASSERT_TRUE(ReadFile(path, buf));
    std::filesystem::remove(path);

    FVE::map_heightHeader heightHdr{};
    std::memcpy(&heightHdr, buf.data() + kHeightHeaderOffset, sizeof(heightHdr));

    EXPECT_TRUE(heightHdr.flags & FVE::kMapHeightAsInt16);
}

// ─── VaryingHeightStoredAsFloat ───────────────────────────────────────────────

TEST(MapFileWriterTests, VaryingHeightStoredAsFloat)
{
    // diff = 3000f  >  kFloatToInt16Limit (2048)  →  raw float
    FVE::AdtGridData grid{};
    grid.V9[0][0] = 3000.f;

    auto path = std::filesystem::temp_directory_path() / "height_float.map";
    ASSERT_TRUE(FVE::MapFileWriter::Write(grid, path, kBuild));

    std::vector<char> buf;
    ASSERT_TRUE(ReadFile(path, buf));
    std::filesystem::remove(path);

    FVE::map_heightHeader heightHdr{};
    std::memcpy(&heightHdr, buf.data() + kHeightHeaderOffset, sizeof(heightHdr));

    EXPECT_FALSE(heightHdr.flags & FVE::kMapHeightAsInt8);
    EXPECT_FALSE(heightHdr.flags & FVE::kMapHeightAsInt16);
}

// ─── SingleAreaIdCompressed ───────────────────────────────────────────────────

TEST(MapFileWriterTests, SingleAreaIdCompressed)
{
    FVE::AdtGridData grid{};
    for (int y = 0; y < FVE::kAdtCellsPerGrid; ++y)
        for (int x = 0; x < FVE::kAdtCellsPerGrid; ++x)
            grid.area_ids[y][x] = 1234;

    auto path = std::filesystem::temp_directory_path() / "area_single.map";
    ASSERT_TRUE(FVE::MapFileWriter::Write(grid, path, kBuild));

    std::vector<char> buf;
    ASSERT_TRUE(ReadFile(path, buf));
    std::filesystem::remove(path);

    FVE::map_areaHeader areaHdr{};
    std::memcpy(&areaHdr, buf.data() + kAreaHeaderOffset, sizeof(areaHdr));

    EXPECT_TRUE(areaHdr.flags & FVE::kMapAreaNoArea);
    EXPECT_EQ(areaHdr.gridArea, 1234);
}

// ─── MultipleAreaIdsWritesFull ────────────────────────────────────────────────

TEST(MapFileWriterTests, MultipleAreaIdsWritesFull)
{
    FVE::AdtGridData grid{};
    grid.area_ids[0][0] = 1;
    grid.area_ids[1][1] = 2;

    auto path = std::filesystem::temp_directory_path() / "area_multi.map";
    ASSERT_TRUE(FVE::MapFileWriter::Write(grid, path, kBuild));

    std::vector<char> buf;
    ASSERT_TRUE(ReadFile(path, buf));
    std::filesystem::remove(path);

    FVE::map_areaHeader areaHdr{};
    std::memcpy(&areaHdr, buf.data() + kAreaHeaderOffset, sizeof(areaHdr));

    EXPECT_FALSE(areaHdr.flags & FVE::kMapAreaNoArea);
}

// ─── TileListContainsCorrectBits ──────────────────────────────────────────────

TEST(MapFileWriterTests, TileListContainsCorrectBits)
{
    constexpr size_t   kTileCount = FVE::kWdtMapSize * FVE::kWdtMapSize; // 4096
    constexpr size_t   kTileIdx   = 3 * FVE::kWdtMapSize + 5;            // y=3,x=5 → 197

    std::bitset<kTileCount> tiles;
    tiles.set(kTileIdx);

    auto path = std::filesystem::temp_directory_path() / "tilelist_bits.tilelist";
    ASSERT_TRUE(FVE::MapFileWriter::WriteTileList(path, tiles, kBuild));

    std::vector<char> buf;
    ASSERT_TRUE(ReadFile(path, buf));
    std::filesystem::remove(path);

    // First 4 bytes: kMapMagic
    uint32_t magic{};
    std::memcpy(&magic, buf.data(), sizeof(magic));
    EXPECT_EQ(magic, FVE::kMapMagic);

    // Bytes 0–3: magic, 4–7: version, 8–11: build, then kTileCount ASCII chars.
    // std::bitset::to_string() maps bit i → string position (N-1-i),
    // so bit kTileIdx appears at string position (kTileCount - 1 - kTileIdx).
    constexpr size_t kBitStringOffset  = sizeof(uint32_t) * 3; // 12
    constexpr size_t kExpectedStrPos   = kTileCount - 1 - kTileIdx;

    ASSERT_GE(buf.size(), kBitStringOffset + kTileCount);
    EXPECT_EQ(buf[kBitStringOffset + kExpectedStrPos], '1');

    // Every other position in the bitstring must be '0'.
    for (size_t i = 0; i < kTileCount; ++i) {
        char expected = (i == kExpectedStrPos) ? '1' : '0';
        EXPECT_EQ(buf[kBitStringOffset + i], expected)
            << "Mismatch at bitstring position " << i;
    }
}
