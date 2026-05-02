#pragma once

#include <shared/game/InventorySlots.h>
#include <array>

namespace Firelands {

struct Bag0InventoryData {
  std::array<uint32_t, kEquipmentSlotCount> equipEntries{};
  std::array<uint32_t, kEquipmentSlotCount> equipGuids{};
  std::array<uint32_t, kEquipmentSlotCount> equipStacks{};
  std::array<uint32_t, kPackSlotCount> packEntries{};
  std::array<uint32_t, kPackSlotCount> packGuids{};
  std::array<uint32_t, kPackSlotCount> packStacks{};
};

}