#pragma once

#include <cstdint>

namespace Firelands {

/// One learned talent row for a character (one spec). `rank` is 0-based, so a
/// stored rank of 0 means the first rank is known. Free talent points are not
/// stored: they are derived as pointsForLevel(level) - spentPoints on load.
struct CharacterTalentRow {
  uint32_t talentId = 0;
  uint8_t rank = 0;
  uint8_t spec = 0;
};

/// One socketed glyph for a character spec. `slot` is the glyph slot index
/// (0..8); `glyph` is the GlyphProperties.dbc id (0 = empty).
struct CharacterGlyphRow {
  uint8_t slot = 0;
  uint32_t glyph = 0;
  uint8_t spec = 0;
};

/// One earned achievement. `earnedDate` is a unix timestamp (sent to the client
/// as a packed time on login).
struct CharacterAchievementRow {
  uint32_t achievementId = 0;
  uint32_t earnedDate = 0;
};

} // namespace Firelands
