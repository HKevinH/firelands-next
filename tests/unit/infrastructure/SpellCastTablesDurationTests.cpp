#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <infrastructure/dbc/SpellCastTablesDbc.h>
#include <vector>

using namespace Firelands;

namespace {

void AppendLeU32(std::vector<uint8_t> &b, uint32_t v) {
  b.push_back(static_cast<uint8_t>(v & 0xFFu));
  b.push_back(static_cast<uint8_t>((v >> 8) & 0xFFu));
  b.push_back(static_cast<uint8_t>((v >> 16) & 0xFFu));
  b.push_back(static_cast<uint8_t>((v >> 24) & 0xFFu));
}

} // namespace

TEST(SpellCastTablesDbcDurationTests, GetDurationMsFromMinimalDbc) {
  std::filesystem::path const tmp =
      std::filesystem::temp_directory_path() / "firelands_ut_spellduration.dbc";

  std::vector<uint8_t> raw;
  raw.insert(raw.end(), {'W', 'D', 'B', 'C'});
  AppendLeU32(raw, 1u);
  AppendLeU32(raw, 4u);
  AppendLeU32(raw, 16u);
  AppendLeU32(raw, 0u);
  AppendLeU32(raw, 7u);
  AppendLeU32(raw, 10000u);
  AppendLeU32(raw, 1000u);
  AppendLeU32(raw, 15000u);

  {
    std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
    ASSERT_TRUE(out);
    out.write(reinterpret_cast<char const *>(raw.data()),
              static_cast<std::streamsize>(raw.size()));
  }

  SpellCastTablesDbc tables;
  ASSERT_TRUE(tables.Load("", "", "", "", "", tmp.string()));
  EXPECT_EQ(tables.GetDurationMs(7u, 1u), 10000u);
  EXPECT_EQ(tables.GetDurationMs(7u, 3u), 12000u);
  EXPECT_EQ(tables.GetDurationMs(7u, 80u), 15000u);
  EXPECT_EQ(tables.GetDurationMs(0u, 1u), 0u);
}
