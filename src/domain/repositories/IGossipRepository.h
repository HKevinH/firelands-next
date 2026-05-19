#pragma once

#include <domain/models/GossipMenu.h>
#include <cstdint>
#include <optional>
#include <vector>

namespace Firelands {

/// Persistence for gossip menus (`gossip_menu`, `gossip_menu_option`).
class IGossipRepository {
public:
  virtual ~IGossipRepository() = default;

  /// Load gossip menu by menu ID (text_id from gossip_menu table).
  virtual std::optional<uint32_t> GetMenuTextId(uint32_t menuId) const = 0;

  /// Load all gossip menu options for a given menu ID.
  virtual std::vector<GossipMenuItem> GetMenuOptions(uint32_t menuId) const = 0;
};

} // namespace Firelands
