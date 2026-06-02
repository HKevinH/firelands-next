#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>

namespace Firelands {

/// Server-side static item data needed for vendor buy/sell, parsed from the
/// client `Item.db2` (class/subclass/display/inventory type) and
/// `Item-sparse.db2` (prices, stack limits, quality) WDB2 files (build 15595).
///
/// Field layout follows the 4.3.4.15595 `ItemEntry` / `ItemSparseEntry` row
/// formats: every column in the on-disk WDB2 record is 4 bytes.
///
///   Item.db2 (8 cols):       ID, Class, SubClass, SoundOverrideSubclass,
///                            Material, DisplayInfoID, InventoryType, SheatheType
///   Item-sparse.db2 (early):  ID, Quality, Flags, Flags2, Unk430_1(f),
///                            Unk430_2(f), BuyCount, BuyPrice, SellPrice,
///                            InventoryType, ... , MaxCount(21), Stackable(22)
///
/// Drop client `Item.db2` + `Item-sparse.db2` (4.3.4.15595) into `data/dbc/`.
/// Without them vendors send empty lists and sell/buy reject (no price data).
class ItemTemplateStore {
public:
  struct Template {
    uint32_t entry = 0;
    uint32_t itemClass = 0;
    uint32_t subClass = 0;
    uint32_t displayId = 0;
    /// Item.db2 InventoryType (slot the item equips into; 0 for non-equippable).
    uint32_t inventoryType = 0;
    uint32_t quality = 0;
    /// Stack size granted per "buy 1" at a vendor (Item-sparse BuyCount; >=1).
    uint32_t buyCount = 1;
    uint32_t buyPrice = 0;
    uint32_t sellPrice = 0;
    /// Item-sparse MaxCount (0 = unlimited carry count).
    int32_t maxCount = 0;
    /// Item-sparse Stackable (max items per stack; >=1).
    int32_t stackable = 1;
  };

  /// Loads `Item.db2` and `Item-sparse.db2` (or `ItemSparse.db2`) under
  /// `dbcDirectory`. Returns true if at least the sparse table (prices) loaded.
  bool load(std::string const &dbcDirectory);

  bool isLoaded() const { return !byEntry_.empty(); }

  std::optional<Template> lookup(uint32_t entry) const;

private:
  std::unordered_map<uint32_t, Template> byEntry_;
};

} // namespace Firelands
