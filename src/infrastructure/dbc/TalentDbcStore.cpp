#include "TalentDbcStore.h"

#include <shared/Logger.h>
#include <shared/dbc/DbcReader.h>

#include <algorithm>
#include <string_view>
#include <utility>

namespace Firelands {

namespace {
// Field layout of client `Talent.dbc` (build 15595): ID, TabID, TierID,
// ColumnIndex, SpellRank[5], PrereqTalent[3], PrereqRank[3], 4 unused columns.
constexpr std::string_view kTalentFmt = "niiiiiiiiiiiiiixxxx";
// `TalentTab.dbc`: ID, (name), (icon), ClassMask, CategoryEnumID, OrderIndex,
// (bg), (desc), (roleMask), MasterySpell[2].
constexpr std::string_view kTalentTabFmt = "nxxiiixxxii";
// `NumTalentsAtLevel.dbc`: Level, NumberOfTalents(float).
constexpr std::string_view kNumTalentsAtLevelFmt = "if";
// `TalentTreePrimarySpells.dbc`: ID, TalentTabID, SpellID, Flags.
constexpr std::string_view kPrimaryTreeSpellsFmt = "diix";
// `GlyphSlot.dbc`: ID, Type, Tooltip.
constexpr std::string_view kGlyphSlotFmt = "nii";
// `GlyphProperties.dbc`: ID, SpellID, Type(GlyphSlotFlags), SpellIconID.
constexpr std::string_view kGlyphPropertiesFmt = "niii";
} // namespace

bool TalentDbcStore::Load(std::string const &talentPath,
                          std::string const &talentTabPath,
                          std::string const &numTalentsAtLevelPath,
                          std::string const &primarySpellsPath) {
  bool const tabs = LoadTalentTabs(talentTabPath);
  bool const talents = LoadTalents(talentPath);
  bool const levels = LoadNumTalentsAtLevel(numTalentsAtLevelPath);
  // Primary-tree spells are optional: without them specialization still sets the
  // tree, it just teaches no signature/mastery spell.
  LoadPrimaryTreeSpells(primarySpellsPath);
  return tabs && talents && levels;
}

bool TalentDbcStore::LoadTalents(std::string const &path) {
  m_talents.clear();
  if (path.empty())
    return false;
  DbcReader reader;
  if (!reader.Load(path)) {
    LOG_WARN("Talent.dbc not found or unreadable: {}", path);
    return false;
  }
  std::vector<uint32_t> const offsets = DbcBuildFieldByteOffsets(kTalentFmt);
  if (!reader.VerifyFormat(kTalentFmt)) {
    LOG_WARN("Talent.dbc: field count mismatch (got {}, expected {}) path={}",
             reader.GetFieldCount(), kTalentFmt.size(), path);
    return false;
  }

  uint32_t const n = reader.GetRecordCount();
  m_talents.reserve(static_cast<size_t>(n));
  for (uint32_t rec = 0; rec < n; ++rec) {
    uint32_t const id = reader.ReadUInt32(rec, 0, offsets);
    if (id == 0u)
      continue;
    TalentEntry e;
    e.id = id;
    e.tabId = reader.ReadUInt32(rec, 1, offsets);
    e.tierId = reader.ReadUInt32(rec, 2, offsets);
    e.columnIndex = reader.ReadUInt32(rec, 3, offsets);
    for (uint8 r = 0; r < kMaxTalentRank; ++r)
      e.spellRank[r] = reader.ReadUInt32(rec, 4 + r, offsets);
    for (uint8 p = 0; p < 3; ++p) {
      e.prereqTalent[p] = reader.ReadUInt32(rec, 9 + p, offsets);
      e.prereqRank[p] = reader.ReadUInt32(rec, 12 + p, offsets);
    }
    m_talents.emplace(id, e);
  }
  LOG_DEBUG("Talent.dbc: {} talents from {}.", m_talents.size(), path);
  return !m_talents.empty();
}

bool TalentDbcStore::LoadTalentTabs(std::string const &path) {
  m_talentTabs.clear();
  if (path.empty())
    return false;
  DbcReader reader;
  if (!reader.Load(path)) {
    LOG_WARN("TalentTab.dbc not found or unreadable: {}", path);
    return false;
  }
  std::vector<uint32_t> const offsets = DbcBuildFieldByteOffsets(kTalentTabFmt);
  if (!reader.VerifyFormat(kTalentTabFmt)) {
    LOG_WARN("TalentTab.dbc: field count mismatch (got {}, expected {}) path={}",
             reader.GetFieldCount(), kTalentTabFmt.size(), path);
    return false;
  }

  uint32_t const n = reader.GetRecordCount();
  m_talentTabs.reserve(static_cast<size_t>(n));
  for (uint32_t rec = 0; rec < n; ++rec) {
    uint32_t const id = reader.ReadUInt32(rec, 0, offsets);
    if (id == 0u)
      continue;
    TalentTabEntry e;
    e.id = id;
    e.classMask = reader.ReadUInt32(rec, 3, offsets);
    e.categoryEnumId = reader.ReadUInt32(rec, 4, offsets);
    e.orderIndex = reader.ReadUInt32(rec, 5, offsets);
    e.masterySpell[0] = reader.ReadUInt32(rec, 9, offsets);
    e.masterySpell[1] = reader.ReadUInt32(rec, 10, offsets);
    m_talentTabs.emplace(id, e);
  }
  LOG_DEBUG("TalentTab.dbc: {} trees from {}.", m_talentTabs.size(), path);
  return !m_talentTabs.empty();
}

bool TalentDbcStore::LoadNumTalentsAtLevel(std::string const &path) {
  m_pointsByLevel.clear();
  if (path.empty())
    return false;
  DbcReader reader;
  if (!reader.Load(path)) {
    LOG_WARN("NumTalentsAtLevel.dbc not found or unreadable: {}", path);
    return false;
  }
  std::vector<uint32_t> const offsets =
      DbcBuildFieldByteOffsets(kNumTalentsAtLevelFmt);
  if (!reader.VerifyFormat(kNumTalentsAtLevelFmt)) {
    LOG_WARN("NumTalentsAtLevel.dbc: field count mismatch (got {}, expected {}) "
             "path={}",
             reader.GetFieldCount(), kNumTalentsAtLevelFmt.size(), path);
    return false;
  }

  uint32_t const n = reader.GetRecordCount();
  m_pointsByLevel.reserve(static_cast<size_t>(n));
  for (uint32_t rec = 0; rec < n; ++rec) {
    uint32_t const level = reader.ReadUInt32(rec, 0, offsets);
    float const points = reader.ReadFloat(rec, 1, offsets);
    if (level == 0u)
      continue;
    m_pointsByLevel.emplace(level, static_cast<uint32>(points + 0.5f));
  }
  LOG_DEBUG("NumTalentsAtLevel.dbc: {} levels from {}.", m_pointsByLevel.size(),
            path);
  return !m_pointsByLevel.empty();
}

TalentEntry const *TalentDbcStore::GetTalent(uint32 talentId) const {
  auto it = m_talents.find(talentId);
  return it == m_talents.end() ? nullptr : &it->second;
}

TalentTabEntry const *TalentDbcStore::GetTalentTab(uint32 tabId) const {
  auto it = m_talentTabs.find(tabId);
  return it == m_talentTabs.end() ? nullptr : &it->second;
}

uint32 TalentDbcStore::GetTalentPointsForLevel(uint8 level) const {
  if (m_pointsByLevel.empty())
    return 0u;
  // The dbc tops out at level 100; clamp so high levels still resolve.
  uint32 key = level;
  if (key > 100u)
    key = 100u;
  auto it = m_pointsByLevel.find(key);
  return it == m_pointsByLevel.end() ? 0u : it->second;
}

bool TalentDbcStore::LoadPrimaryTreeSpells(std::string const &path) {
  m_primaryTreeSpells.clear();
  if (path.empty())
    return false;
  DbcReader reader;
  if (!reader.Load(path)) {
    LOG_WARN("TalentTreePrimarySpells.dbc not found or unreadable: {}", path);
    return false;
  }
  std::vector<uint32_t> const offsets =
      DbcBuildFieldByteOffsets(kPrimaryTreeSpellsFmt);
  if (!reader.VerifyFormat(kPrimaryTreeSpellsFmt)) {
    LOG_WARN("TalentTreePrimarySpells.dbc: field count mismatch (got {}, "
             "expected {}) path={}",
             reader.GetFieldCount(), kPrimaryTreeSpellsFmt.size(), path);
    return false;
  }

  uint32_t const n = reader.GetRecordCount();
  for (uint32_t rec = 0; rec < n; ++rec) {
    uint32_t const tabId = reader.ReadUInt32(rec, 1, offsets);
    uint32_t const spellId = reader.ReadUInt32(rec, 2, offsets);
    if (tabId == 0u || spellId == 0u)
      continue;
    m_primaryTreeSpells[tabId].push_back(spellId);
  }
  LOG_DEBUG("TalentTreePrimarySpells.dbc: {} trees with signature spells from {}.",
            m_primaryTreeSpells.size(), path);
  return !m_primaryTreeSpells.empty();
}

std::vector<uint32> TalentDbcStore::GetClassTalentTabs(uint8 classId) const {
  std::vector<std::pair<uint32, uint32>> ordered; // (orderIndex, tabId)
  uint32 const classMask = (classId > 0) ? (1u << (classId - 1)) : 0u;
  for (auto const &[id, tab] : m_talentTabs) {
    (void)id;
    if ((tab.classMask & classMask) != 0u)
      ordered.emplace_back(tab.orderIndex, tab.id);
  }
  std::sort(ordered.begin(), ordered.end());
  std::vector<uint32> tabs;
  tabs.reserve(ordered.size());
  for (auto const &[order, tabId] : ordered) {
    (void)order;
    tabs.push_back(tabId);
  }
  return tabs;
}

std::vector<uint32> const *
TalentDbcStore::GetPrimaryTreeSpells(uint32 tabId) const {
  auto it = m_primaryTreeSpells.find(tabId);
  return it == m_primaryTreeSpells.end() ? nullptr : &it->second;
}

bool TalentDbcStore::LoadGlyphs(std::string const &glyphSlotPath,
                                std::string const &glyphPropertiesPath) {
  bool const slots = LoadGlyphSlots(glyphSlotPath);
  bool const props = LoadGlyphProperties(glyphPropertiesPath);
  return slots && props;
}

bool TalentDbcStore::LoadGlyphSlots(std::string const &path) {
  m_glyphSlotIds.clear();
  m_glyphSlotType.clear();
  if (path.empty())
    return false;
  DbcReader reader;
  if (!reader.Load(path)) {
    LOG_WARN("GlyphSlot.dbc not found or unreadable: {}", path);
    return false;
  }
  std::vector<uint32_t> const offsets = DbcBuildFieldByteOffsets(kGlyphSlotFmt);
  if (!reader.VerifyFormat(kGlyphSlotFmt)) {
    LOG_WARN("GlyphSlot.dbc: field count mismatch (got {}, expected {}) path={}",
             reader.GetFieldCount(), kGlyphSlotFmt.size(), path);
    return false;
  }
  uint32_t const n = reader.GetRecordCount();
  for (uint32_t rec = 0; rec < n; ++rec) {
    uint32_t const id = reader.ReadUInt32(rec, 0, offsets);
    uint32_t const type = reader.ReadUInt32(rec, 1, offsets);
    if (id == 0u)
      continue;
    m_glyphSlotIds.push_back(id); // DBC row order = slot index order
    m_glyphSlotType.emplace(id, type);
  }
  LOG_DEBUG("GlyphSlot.dbc: {} slots from {}.", m_glyphSlotIds.size(), path);
  return !m_glyphSlotIds.empty();
}

bool TalentDbcStore::LoadGlyphProperties(std::string const &path) {
  m_glyphProperties.clear();
  if (path.empty())
    return false;
  DbcReader reader;
  if (!reader.Load(path)) {
    LOG_WARN("GlyphProperties.dbc not found or unreadable: {}", path);
    return false;
  }
  std::vector<uint32_t> const offsets =
      DbcBuildFieldByteOffsets(kGlyphPropertiesFmt);
  if (!reader.VerifyFormat(kGlyphPropertiesFmt)) {
    LOG_WARN("GlyphProperties.dbc: field count mismatch (got {}, expected {}) "
             "path={}",
             reader.GetFieldCount(), kGlyphPropertiesFmt.size(), path);
    return false;
  }
  uint32_t const n = reader.GetRecordCount();
  m_glyphProperties.reserve(static_cast<size_t>(n));
  for (uint32_t rec = 0; rec < n; ++rec) {
    uint32_t const id = reader.ReadUInt32(rec, 0, offsets);
    if (id == 0u)
      continue;
    GlyphPropertiesEntry e;
    e.id = id;
    e.spellId = reader.ReadUInt32(rec, 1, offsets);
    e.type = reader.ReadUInt32(rec, 2, offsets);
    m_glyphProperties.emplace(id, e);
  }
  LOG_DEBUG("GlyphProperties.dbc: {} glyphs from {}.", m_glyphProperties.size(),
            path);
  return !m_glyphProperties.empty();
}

uint32 TalentDbcStore::GetGlyphSlotType(uint32 slotId) const {
  auto it = m_glyphSlotType.find(slotId);
  return it == m_glyphSlotType.end() ? UINT32_MAX : it->second;
}

GlyphPropertiesEntry const *
TalentDbcStore::GetGlyphProperties(uint32 glyphId) const {
  auto it = m_glyphProperties.find(glyphId);
  return it == m_glyphProperties.end() ? nullptr : &it->second;
}

bool TalentDbcStore::LoadGlyphApplySpells(std::string const &spellEffectPath) {
  m_glyphApplySpellToProp.clear();
  if (spellEffectPath.empty())
    return false;
  // `SpellEffect.dbc` 4.3.4 layout (same fmt the spell store uses): Effect at
  // field 1, EffectMiscValue at 16, SpellID at 24.
  constexpr std::string_view kFmt = "nifiiiffiiiiiifiifiiiiiiiix";
  constexpr uint32_t kFieldEffect = 1;
  constexpr uint32_t kFieldMiscValue = 16;
  constexpr uint32_t kFieldSpellId = 24;
  constexpr uint32_t kEffectApplyGlyph = 74;

  DbcReader reader;
  if (!reader.Load(spellEffectPath)) {
    LOG_WARN("SpellEffect.dbc not readable for glyph map: {}", spellEffectPath);
    return false;
  }
  std::vector<uint32_t> const offsets = DbcBuildFieldByteOffsets(kFmt);
  if (!reader.VerifyFormat(kFmt)) {
    LOG_WARN("SpellEffect.dbc: field count mismatch for glyph map (got {}, "
             "expected {})",
             reader.GetFieldCount(), kFmt.size());
    return false;
  }
  uint32_t const n = reader.GetRecordCount();
  for (uint32_t rec = 0; rec < n; ++rec) {
    if (reader.ReadUInt32(rec, kFieldEffect, offsets) != kEffectApplyGlyph)
      continue;
    uint32_t const spellId = reader.ReadUInt32(rec, kFieldSpellId, offsets);
    uint32_t const glyphId = reader.ReadUInt32(rec, kFieldMiscValue, offsets);
    if (spellId != 0u && glyphId != 0u)
      m_glyphApplySpellToProp.emplace(spellId, glyphId);
  }
  LOG_DEBUG("Glyph apply spells: {} mappings from {}.",
            m_glyphApplySpellToProp.size(), spellEffectPath);
  return !m_glyphApplySpellToProp.empty();
}

uint32 TalentDbcStore::GetGlyphForApplySpell(uint32 spellId) const {
  auto it = m_glyphApplySpellToProp.find(spellId);
  return it == m_glyphApplySpellToProp.end() ? 0u : it->second;
}

} // namespace Firelands
