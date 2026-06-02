#include <shared/dbc/ItemTemplateStore.h>
#include <shared/Logger.h>

#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <vector>

namespace Firelands {

namespace {

constexpr uint32_t kWdb2Magic =
    (uint32_t('W')) | (uint32_t('D') << 8) | (uint32_t('B') << 16) |
    (uint32_t('2') << 24);

uint32_t ReadU32Le(std::vector<uint8_t> const &data, size_t offset) {
  if (offset + sizeof(uint32_t) > data.size())
    return 0;
  uint32_t v = 0;
  std::memcpy(&v, data.data() + offset, sizeof(v));
  return v;
}

/// Walks a WDB2 file (build > 12880 extended header + optional index table) and
/// invokes `onRow(id, rowBytes, recordSize)` for each record keyed by its first
/// uint32. Mirrors the loaders in `ItemDb2Wdb2` / `ItemDbHotfixStore`.
bool ForEachWdb2Row(
    std::string const &path, uint32_t minRecordSize,
    std::function<void(uint32_t, std::vector<uint8_t> const &, uint32_t)> const
        &onRow) {
  std::ifstream in(path, std::ios::binary);
  if (!in)
    return false;

  std::vector<uint8_t> raw((std::istreambuf_iterator<char>(in)),
                           std::istreambuf_iterator<char>());
  if (raw.size() < 48 || ReadU32Le(raw, 0) != kWdb2Magic) {
    LOG_WARN("ItemTemplateStore: invalid or tiny WDB2 file {}", path);
    return false;
  }

  uint32_t const recordCount = ReadU32Le(raw, 4);
  uint32_t const fieldCount = ReadU32Le(raw, 8);
  uint32_t const recordSize = ReadU32Le(raw, 12);
  uint32_t const stringSize = ReadU32Le(raw, 16);
  uint32_t const build = ReadU32Le(raw, 24);

  size_t pos = 32;
  uint32_t minIndex = 0;
  uint32_t maxIndex = 0;
  if (build > 12880) {
    if (raw.size() < pos + 16) {
      LOG_WARN("ItemTemplateStore: truncated extended header in {}", path);
      return false;
    }
    minIndex = ReadU32Le(raw, pos);
    pos += 4;
    maxIndex = ReadU32Le(raw, pos);
    pos += 4;
    pos += 4; // locale
    pos += 4; // unk5
  }

  if (maxIndex != 0) {
    uint64_t const span =
        static_cast<uint64_t>(maxIndex) - static_cast<uint64_t>(minIndex) + 1u;
    uint64_t const skip = span * 4u + span * 2u;
    if (raw.size() < pos + skip) {
      LOG_WARN("ItemTemplateStore: truncated index table in {}", path);
      return false;
    }
    pos += static_cast<size_t>(skip);
  }

  if (fieldCount < 1 || recordSize < minRecordSize || recordCount == 0) {
    LOG_WARN("ItemTemplateStore: unexpected layout (fields={} recordSize={} "
             "count={} min={}): {}",
             fieldCount, recordSize, recordCount, minRecordSize, path);
    return false;
  }

  uint64_t const dataBytes =
      static_cast<uint64_t>(recordCount) * static_cast<uint64_t>(recordSize) +
      static_cast<uint64_t>(stringSize);
  if (raw.size() < pos + dataBytes) {
    LOG_WARN("ItemTemplateStore: truncated data in {}", path);
    return false;
  }

  for (uint32_t ri = 0; ri < recordCount; ++ri) {
    size_t const rec =
        pos + static_cast<size_t>(ri) * static_cast<size_t>(recordSize);
    uint32_t const id = ReadU32Le(raw, rec);
    if (id == 0)
      continue;
    std::vector<uint8_t> row(recordSize);
    std::memcpy(row.data(), raw.data() + rec, recordSize);
    onRow(id, row, recordSize);
  }
  return true;
}

// Item.db2 column byte offsets (4-byte columns).
constexpr size_t kItemOffClass = 1u * 4u;
constexpr size_t kItemOffSubClass = 2u * 4u;
constexpr size_t kItemOffDisplay = 5u * 4u;
constexpr size_t kItemOffInvType = 6u * 4u;

// Item-sparse.db2 column byte offsets (4-byte columns).
constexpr size_t kSparseOffQuality = 1u * 4u;
constexpr size_t kSparseOffBuyCount = 6u * 4u;
constexpr size_t kSparseOffBuyPrice = 7u * 4u;
constexpr size_t kSparseOffSellPrice = 8u * 4u;
constexpr size_t kSparseOffInvType = 9u * 4u;
constexpr size_t kSparseOffMaxCount = 21u * 4u;
constexpr size_t kSparseOffStackable = 22u * 4u;

} // namespace

bool ItemTemplateStore::load(std::string const &dbcDirectory) {
  byEntry_.clear();

  std::filesystem::path const base(dbcDirectory);

  // Item.db2: class / subclass / display / inventory type. Needs >= 7 columns.
  bool const itemOk = ForEachWdb2Row(
      (base / "Item.db2").string(), /*minRecordSize=*/28,
      [&](uint32_t id, std::vector<uint8_t> const &row, uint32_t) {
        Template &t = byEntry_[id];
        t.entry = id;
        t.itemClass = ReadU32Le(row, kItemOffClass);
        t.subClass = ReadU32Le(row, kItemOffSubClass);
        t.displayId = ReadU32Le(row, kItemOffDisplay);
        t.inventoryType = ReadU32Le(row, kItemOffInvType);
      });

  // Item-sparse.db2: prices, stack limits, quality. Needs columns up to
  // Stackable(22) -> 23 columns -> 92 bytes.
  std::string const sparsePathA = (base / "Item-sparse.db2").string();
  std::string const sparsePathB = (base / "ItemSparse.db2").string();

  size_t invTypeChecked = 0;
  size_t invTypeMismatches = 0;
  auto onSparse = [&](uint32_t id, std::vector<uint8_t> const &row, uint32_t) {
    Template &t = byEntry_[id];
    t.entry = id;
    t.quality = ReadU32Le(row, kSparseOffQuality);
    t.buyCount = ReadU32Le(row, kSparseOffBuyCount);
    if (t.buyCount == 0)
      t.buyCount = 1;
    t.buyPrice = ReadU32Le(row, kSparseOffBuyPrice);
    t.sellPrice = ReadU32Le(row, kSparseOffSellPrice);
    t.maxCount = static_cast<int32_t>(ReadU32Le(row, kSparseOffMaxCount));
    t.stackable = static_cast<int32_t>(ReadU32Le(row, kSparseOffStackable));
    if (t.stackable <= 0)
      t.stackable = 1;

    // Cross-check: Item.db2 InventoryType (col 6) must match Item-sparse
    // InventoryType (col 9). A high mismatch rate means the sparse column
    // offsets are misaligned for this client build -> prices would be garbage.
    if (t.displayId != 0 || t.inventoryType != 0) {
      uint32_t const sparseInv = ReadU32Le(row, kSparseOffInvType);
      ++invTypeChecked;
      if (sparseInv != t.inventoryType)
        ++invTypeMismatches;
    }
  };

  bool sparseOk = ForEachWdb2Row(sparsePathA, /*minRecordSize=*/92, onSparse);
  if (!sparseOk)
    sparseOk = ForEachWdb2Row(sparsePathB, /*minRecordSize=*/92, onSparse);

  if (!itemOk)
    LOG_WARN("ItemTemplateStore: Item.db2 missing/invalid under {} — vendor "
             "display ids unavailable.",
             dbcDirectory);
  if (!sparseOk)
    LOG_WARN("ItemTemplateStore: Item-sparse.db2 missing/invalid under {} — "
             "vendor prices unavailable (buy/sell disabled).",
             dbcDirectory);

  if (invTypeChecked > 0 && invTypeMismatches * 100u > invTypeChecked * 5u) {
    LOG_WARN("ItemTemplateStore: Item-sparse InventoryType mismatched Item.db2 "
             "on {}/{} entries — sparse column layout may be wrong for this "
             "client build; prices are likely incorrect.",
             invTypeMismatches, invTypeChecked);
  }

  if (sparseOk) {
    LOG_DEBUG("ItemTemplateStore: ready ({} entries, invType cross-check "
              "{}/{} matched).",
              byEntry_.size(), invTypeChecked - invTypeMismatches,
              invTypeChecked);
  }

  return sparseOk;
}

std::optional<ItemTemplateStore::Template>
ItemTemplateStore::lookup(uint32_t entry) const {
  auto it = byEntry_.find(entry);
  if (it == byEntry_.end())
    return std::nullopt;
  return it->second;
}

} // namespace Firelands
