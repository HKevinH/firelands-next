#include <gtest/gtest.h>
#include <shared/Logger.h>
#include <shared/dbc/ItemTemplateStore.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <vector>

namespace {

void AppendU32(std::vector<uint8_t> &b, uint32_t v) {
  b.push_back(static_cast<uint8_t>(v & 0xFFu));
  b.push_back(static_cast<uint8_t>((v >> 8) & 0xFFu));
  b.push_back(static_cast<uint8_t>((v >> 16) & 0xFFu));
  b.push_back(static_cast<uint8_t>((v >> 24) & 0xFFu));
}

/// Writes a minimal WDB2 (build 15595, no index table) with one record built
/// from `columns`, padded/truncated to `recordSize` bytes.
void WriteWdb2(std::filesystem::path const &p, uint32_t fieldCount,
               uint32_t recordSize, std::vector<uint32_t> const &columns) {
  std::vector<uint8_t> raw;
  AppendU32(raw, (uint32_t('W')) | (uint32_t('D') << 8) | (uint32_t('B') << 16) |
                     (uint32_t('2') << 24));
  AppendU32(raw, 1u);          // recordCount
  AppendU32(raw, fieldCount);  // fieldCount
  AppendU32(raw, recordSize);  // recordSize
  AppendU32(raw, 0u);          // stringSize
  AppendU32(raw, 0u);          // tableHash
  AppendU32(raw, 15595u);      // build (> 12880 -> extended header)
  AppendU32(raw, 0u);          // unused
  // Extended header (build > 12880): minIndex, maxIndex, locale, unk5.
  AppendU32(raw, 0u);          // minIndex
  AppendU32(raw, 0u);          // maxIndex (0 -> no index table)
  AppendU32(raw, 0u);          // locale
  AppendU32(raw, 0u);          // unk5

  std::vector<uint8_t> rec;
  for (uint32_t c : columns)
    AppendU32(rec, c);
  rec.resize(recordSize, 0u);
  raw.insert(raw.end(), rec.begin(), rec.end());

  std::ofstream out(p, std::ios::binary);
  out.write(reinterpret_cast<char const *>(raw.data()),
            static_cast<std::streamsize>(raw.size()));
}

class ItemTemplateStoreTest : public ::testing::Test {
protected:
  void SetUp() override {
    using namespace Firelands;
    if (!Logger::IsInitialized())
      Logger::Init(LoggerBuilder().WithConsole(false).Build());
    dir_ = std::filesystem::temp_directory_path() / "firelands_itemtpl_unit";
    std::filesystem::create_directories(dir_);
  }
  void TearDown() override {
    std::error_code ec;
    std::filesystem::remove_all(dir_, ec);
  }
  std::filesystem::path dir_;
};

} // namespace

TEST_F(ItemTemplateStoreTest, ParsesPricesAndDisplayFromBothFiles) {
  using namespace Firelands;

  // Item.db2: ID, Class, SubClass, SoundOverrideSubclass, Material,
  //           DisplayInfoID, InventoryType, SheatheType
  WriteWdb2(dir_ / "Item.db2", /*fieldCount=*/8, /*recordSize=*/32,
            {2488u, 4u, 1u, 0xFFFFFFFFu, 7u, 12345u, 1u, 0u});

  // Item-sparse.db2: ID, Quality, Flags, Flags2, Unk430_1, Unk430_2,
  //   BuyCount(6), BuyPrice(7), SellPrice(8), InventoryType(9), ... ,
  //   MaxCount(21), Stackable(22). Cols 10..20 zero-filled by resize.
  std::vector<uint32_t> sparse(23, 0u);
  sparse[0] = 2488u;   // ID
  sparse[1] = 2u;      // Quality
  sparse[6] = 1u;      // BuyCount
  sparse[7] = 100u;    // BuyPrice
  sparse[8] = 25u;     // SellPrice
  sparse[9] = 1u;      // InventoryType (matches Item.db2 -> cross-check passes)
  sparse[21] = 0u;     // MaxCount (unlimited)
  sparse[22] = 20u;    // Stackable
  WriteWdb2(dir_ / "Item-sparse.db2", /*fieldCount=*/23, /*recordSize=*/92,
            sparse);

  ItemTemplateStore store;
  ASSERT_TRUE(store.load(dir_.string()));

  auto t = store.lookup(2488u);
  ASSERT_TRUE(t);
  EXPECT_EQ(t->entry, 2488u);
  EXPECT_EQ(t->itemClass, 4u);
  EXPECT_EQ(t->subClass, 1u);
  EXPECT_EQ(t->displayId, 12345u);
  EXPECT_EQ(t->inventoryType, 1u);
  EXPECT_EQ(t->quality, 2u);
  EXPECT_EQ(t->buyCount, 1u);
  EXPECT_EQ(t->buyPrice, 100u);
  EXPECT_EQ(t->sellPrice, 25u);
  EXPECT_EQ(t->maxCount, 0);
  EXPECT_EQ(t->stackable, 20);
}

TEST_F(ItemTemplateStoreTest, FailsWhenSparseMissing) {
  using namespace Firelands;
  WriteWdb2(dir_ / "Item.db2", 8, 32,
            {1u, 0u, 0u, 0u, 0u, 0u, 0u, 0u});
  ItemTemplateStore store;
  // No prices -> not usable for vendor.
  EXPECT_FALSE(store.load(dir_.string()));
}

TEST_F(ItemTemplateStoreTest, BuyCountAndStackableClampToOne) {
  using namespace Firelands;
  std::vector<uint32_t> sparse(23, 0u);
  sparse[0] = 55u;
  sparse[6] = 0u;  // BuyCount 0 -> clamps to 1
  sparse[22] = 0u; // Stackable 0 -> clamps to 1
  WriteWdb2(dir_ / "Item-sparse.db2", 23, 92, sparse);

  ItemTemplateStore store;
  ASSERT_TRUE(store.load(dir_.string()));
  auto t = store.lookup(55u);
  ASSERT_TRUE(t);
  EXPECT_EQ(t->buyCount, 1u);
  EXPECT_EQ(t->stackable, 1);
}
