#pragma once

#include <cstdint>
#include <vector>

namespace Firelands {

/// One row of a creature's `npc_vendor` list (what the NPC offers for sale).
struct VendorItemEntry {
  /// `item_template` entry sold in this slot.
  uint32_t item = 0;
  /// Max in stock before restock (0 = unlimited / always available).
  int32_t maxCount = 0;
  /// Restock interval in seconds (0 = no restock tracking).
  uint32_t incrTime = 0;
  /// `ItemExtendedCost.dbc` id (0 = pure money cost). Not honored yet.
  uint32_t extendedCost = 0;
  /// 1 = item, 2 = currency. Vendors here only sell type 1 for now.
  uint8_t type = 1;
};

/// Persistence for `npc_vendor` (the item list a vendor creature offers).
class IVendorRepository {
public:
  virtual ~IVendorRepository() = default;

  /// Vendor list for a creature template entry, ordered by `slot`.
  virtual std::vector<VendorItemEntry>
  GetVendorItems(uint32_t vendorEntry) const = 0;
};

} // namespace Firelands
