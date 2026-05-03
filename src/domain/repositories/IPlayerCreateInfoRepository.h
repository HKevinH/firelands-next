#pragma once

#include <domain/models/PlayerCreateInfo.h>
#include <domain/models/PlayerTemplateStats.h>
#include <cstdint>
#include <optional>
#include <vector>

namespace Firelands {

class IPlayerCreateInfoRepository {
public:
  virtual ~IPlayerCreateInfoRepository() = default;

  virtual std::optional<PlayerCreateInfo>
  GetStartPosition(uint8 race, uint8 klass) = 0;

  virtual std::vector<PlayerCreateVisualItem>
  GetVisualItems(uint8 race, uint8 klass, uint8 gender, uint8 outfitId) = 0;

  /// Rows from `playercreateinfo_item` (reference merges after CharStartOutfit).
  virtual std::vector<StarterItemGrant>
  GetExtraCreateItems(uint8 race, uint8 klass) = 0;

  /// Spell IDs from `playercreateinfo_spell` (race/class wildcards 0), same merge
  /// rules as Trinity `playercreateinfo_spell` / `PlayerCreateInfo`.
  virtual std::vector<uint32_t> GetStarterSpells(uint8_t race,
                                                 uint8_t klass) = 0;

  /// Trinity-style `player_classlevelstats`; empty when row missing.
  virtual std::optional<PlayerClassLevelStats>
  GetClassLevelStats(uint8_t klass, uint8_t level) = 0;

  /// Trinity-style `player_racestats`; empty when row missing (treated as zeros).
  virtual std::optional<PlayerRaceStats> GetRaceStats(uint8_t race) = 0;

  /// Trinity `player_xp_for_level`: XP to advance from `currentLevel` to
  /// `currentLevel + 1` (levels 1..84). Returns 0 if the table/row is missing.
  virtual uint32_t GetXpForNextLevel(uint8_t currentLevel) const = 0;
};

} // namespace Firelands
