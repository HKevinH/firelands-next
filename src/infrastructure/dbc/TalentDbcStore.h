#pragma once

#include <shared/Common.h>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace Firelands {

/// Max ranks a single talent can have (client 4.3.4: SpellRank[5]).
constexpr uint8 kMaxTalentRank = 5;

/// Points that must be spent in the active spec's primary tree before talents
/// in any other tree of that spec can be learned (Cataclysm 31-point trees).
constexpr uint32 kReqPrimaryTreeTalents = 31;

/// One row of client `Talent.dbc` (build 15595).
/// fmt "niiiiiiiiiiiiiixxxx": ID, TabID, TierID, ColumnIndex, SpellRank[5],
/// PrereqTalent[3], PrereqRank[3], then 4 unused columns.
struct TalentEntry {
  uint32 id = 0;
  uint32 tabId = 0;       ///< index into TalentTab.dbc
  uint32 tierId = 0;      ///< row within the tree; gates min points spent
  uint32 columnIndex = 0;
  uint32 spellRank[kMaxTalentRank] = {0, 0, 0, 0, 0};
  uint32 prereqTalent[3] = {0, 0, 0};
  uint32 prereqRank[3] = {0, 0, 0};
};

/// One row of client `TalentTab.dbc` (build 15595).
/// fmt "nxxiiixxxii": ID, ClassMask, CategoryEnumID, OrderIndex, MasterySpell[2].
struct TalentTabEntry {
  uint32 id = 0;
  uint32 classMask = 0;       ///< bitmask of classes that may use this tree
  uint32 categoryEnumId = 0;
  uint32 orderIndex = 0;      ///< 0/1/2 within the class (primary tree id space)
  uint32 masterySpell[2] = {0, 0};
};

/// 4.3.4 glyph slots: 9 total (3 prime, 3 major, 3 minor). Index→min level:
/// 0/1/6→25, 2/3/7→50, 4/5/8→75.
constexpr uint8 kMaxGlyphSlots = 9;

/// One row of client `GlyphProperties.dbc` (fmt "niii": ID, SpellID, Type, Icon).
/// `type` must match the target slot's type for the glyph to be applicable.
struct GlyphPropertiesEntry {
  uint32 id = 0;
  uint32 spellId = 0;  ///< passive aura applied while the glyph is socketed
  uint32 type = 0;     ///< 0=major, 1=minor, 2=prime (matches GlyphSlot.Type)
};

/// Loads the three DBC files backing the talent system. Each file is optional;
/// a missing/invalid file logs a warning and leaves that table empty (the
/// permissive fallback keeps the server up with talents simply unavailable).
class TalentDbcStore {
public:
  bool Load(std::string const &talentPath, std::string const &talentTabPath,
            std::string const &numTalentsAtLevelPath,
            std::string const &primarySpellsPath);

  /// Loads `GlyphSlot.dbc` (slot id + type, in row order) and
  /// `GlyphProperties.dbc` (glyph id → spell + type). Optional; on failure the
  /// glyph getters simply return empty/zero.
  bool LoadGlyphs(std::string const &glyphSlotPath,
                  std::string const &glyphPropertiesPath);

  /// The (up to 9) glyph slot ids in DBC row order; index = glyph slot index.
  std::vector<uint32> const &GlyphSlotIds() const { return m_glyphSlotIds; }
  /// Glyph type of a slot id (from GlyphSlot.dbc), or UINT32_MAX if unknown.
  uint32 GetGlyphSlotType(uint32 slotId) const;
  GlyphPropertiesEntry const *GetGlyphProperties(uint32 glyphId) const;

  /// Builds the map "glyph-apply spell id → GlyphProperties id" from
  /// `SpellEffect.dbc` (effect 74 = SPELL_EFFECT_APPLY_GLYPH; MiscValue is the
  /// glyph id). Lets the cast handler recognise a glyph item's apply spell.
  bool LoadGlyphApplySpells(std::string const &spellEffectPath);
  /// GlyphProperties id applied by casting `spellId`, or 0 if it isn't a glyph
  /// apply spell.
  uint32 GetGlyphForApplySpell(uint32 spellId) const;

  bool IsLoaded() const { return !m_talents.empty(); }
  size_t TalentCount() const { return m_talents.size(); }

  TalentEntry const *GetTalent(uint32 talentId) const;
  TalentTabEntry const *GetTalentTab(uint32 tabId) const;

  /// The (usually 3) talent tree ids usable by `classId` (1-based), ordered by
  /// the tree's OrderIndex (0=first tab). Empty when the class has no trees.
  std::vector<uint32> GetClassTalentTabs(uint8 classId) const;

  /// Signature/mastery spells taught when a tree becomes the primary spec
  /// (`TalentTreePrimarySpells.dbc`). Returns nullptr when none.
  std::vector<uint32> const *GetPrimaryTreeSpells(uint32 tabId) const;

  /// All talents, for callers that must scan every row (e.g. tallying points
  /// already spent in a tree). Keyed by talent id.
  std::unordered_map<uint32, TalentEntry> const &AllTalents() const {
    return m_talents;
  }

  /// Total talent points a non-Death-Knight of `level` should have, from
  /// `NumTalentsAtLevel.dbc`. Returns 0 when the table is unavailable.
  uint32 GetTalentPointsForLevel(uint8 level) const;

private:
  bool LoadTalents(std::string const &path);
  bool LoadTalentTabs(std::string const &path);
  bool LoadNumTalentsAtLevel(std::string const &path);
  bool LoadPrimaryTreeSpells(std::string const &path);
  bool LoadGlyphSlots(std::string const &path);
  bool LoadGlyphProperties(std::string const &path);

  std::unordered_map<uint32, TalentEntry> m_talents;
  std::unordered_map<uint32, TalentTabEntry> m_talentTabs;
  /// level (1..100) → number of talent points available at that level.
  std::unordered_map<uint32, uint32> m_pointsByLevel;
  /// talent tree id → signature/mastery spells learned on specialization.
  std::unordered_map<uint32, std::vector<uint32>> m_primaryTreeSpells;
  /// Glyph slot ids in DBC row order (slot index 0..8).
  std::vector<uint32> m_glyphSlotIds;
  /// slot id → slot type (GlyphSlot.Type).
  std::unordered_map<uint32, uint32> m_glyphSlotType;
  /// glyph id → properties (spell + type).
  std::unordered_map<uint32, GlyphPropertiesEntry> m_glyphProperties;
  /// glyph-apply spell id → GlyphProperties id (from SpellEffect.dbc effect 74).
  std::unordered_map<uint32, uint32> m_glyphApplySpellToProp;
};

} // namespace Firelands
