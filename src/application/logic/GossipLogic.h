#pragma once

#include <domain/models/GossipMenu.h>
#include <cstdint>
#include <vector>

namespace Firelands {

/// Default gossip menu used when `creature_template.gossip_menu_id` is zero; options are
/// filtered by `OptionNpcflag` vs template `npcflag` (Trinity-style role shortcuts).
inline constexpr uint32_t kDefaultNpcGossipMenuId = 0u;

/// Picks the menu id to load from `gossip_menu` for a creature template.
inline uint32_t ResolveGossipMenuIdForTemplate(uint32_t templateGossipMenuId) noexcept {
  return templateGossipMenuId != 0 ? templateGossipMenuId : kDefaultNpcGossipMenuId;
}

/// Keeps options with no npcflag gate or a matching role flag on the template.
inline std::vector<GossipMenuItem>
FilterGossipOptionsByNpcFlags(std::vector<GossipMenuItem> items,
                              uint64_t templateNpcFlags) {
  if (templateNpcFlags == 0)
    return items;
  std::vector<GossipMenuItem> filtered;
  filtered.reserve(items.size());
  for (auto &item : items) {
    if (item.optionNpcflag == 0 ||
        (item.optionNpcflag & templateNpcFlags) != 0)
      filtered.push_back(std::move(item));
  }
  return filtered;
}

inline const GossipMenuItem *
FindGossipMenuItem(std::vector<GossipMenuItem> const &items,
                   uint32_t optionIndex) noexcept {
  for (auto const &item : items) {
    if (item.optionIndex == optionIndex)
      return &item;
  }
  return nullptr;
}

} // namespace Firelands
