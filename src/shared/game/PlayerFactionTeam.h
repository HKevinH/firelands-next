#pragma once

#include <cstdint>

namespace Firelands {

/// Cataclysm (4.3.4) playable races → Alliance / Horde for spell-range band hints when both
/// units are players. Does not replace `FactionTemplate.dbc` reaction (PvP flags, sanctuary).
/// Unknown races leave `outSameTeam` unset and return false.
inline bool TrySpellRangeFriendlyTeamHint(uint8 casterRace, uint8 targetRace,
                                          bool *outSameTeam) {
  if (!outSameTeam)
    return false;

  enum class Team : uint8 { Unknown, Alliance, Horde };
  auto teamOf = [](uint8 race) -> Team {
    switch (race) {
    case 1:  // Human
    case 3:  // Dwarf
    case 4:  // Night Elf
    case 7:  // Gnome
    case 11: // Draenei
    case 22: // Worgen
      return Team::Alliance;
    case 2:  // Orc
    case 5:  // Undead
    case 6:  // Tauren
    case 8:  // Troll
    case 9:  // Goblin
    case 10: // Blood Elf
      return Team::Horde;
    default:
      return Team::Unknown;
    }
  };

  Team const a = teamOf(casterRace);
  Team const b = teamOf(targetRace);
  if (a == Team::Unknown || b == Team::Unknown)
    return false;
  *outSameTeam = (a == b);
  return true;
}

} // namespace Firelands
