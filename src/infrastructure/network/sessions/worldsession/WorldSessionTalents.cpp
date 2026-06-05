#include <application/services/CharacterService.h>
#include <application/services/WorldService.h>
#include <application/spell/SpellHitEffects.h>
#include <application/spell/SpellManager.h>
#include <domain/repositories/ISpellDefinitionStore.h>
#include <domain/world/Map.h>
#include <domain/world/Player.h>
#include <infrastructure/dbc/TalentDbcStore.h>
#include <infrastructure/network/sessions/WorldSession.h>
#include <infrastructure/network/sessions/worldsession/WorldSessionSpellEffects.h>
#include <shared/Logger.h>
#include <shared/game/ActionButton.h>
#include <shared/network/UpdateData.h>
#include <shared/network/UpdateFields.h>
#include <shared/network/WorldOpcodes.h>
#include <shared/network/WorldPacket.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <map>
#include <optional>
#include <vector>

namespace Firelands {

void WorldSession::RecalculateTalentPoints() {
  uint32 total = 0;
  if (auto store = WorldService::Instance().GetTalentStore())
    total = store->GetTalentPointsForLevel(_playerLevel);

  uint32 spent = 0;
  for (CharacterTalentRow const &t : _characterTalents)
    spent += static_cast<uint32>(t.rank) + 1u; // rank is 0-based

  _talentFreePoints = (total > spent) ? (total - spent) : 0u;
  LOG_DEBUG("[TALENT] recalc: level={} total={} spent={} free={} storeLoaded={}",
            _playerLevel, total, spent, _talentFreePoints,
            WorldService::Instance().GetTalentStore() != nullptr);
}

void WorldSession::LoadTalentsForCharacter(uint32 characterGuid) {
  _characterTalents.clear();
  uint8 const spec = _activeActionBarSpec;
  if (_charService && characterGuid != 0u) {
    _characterTalents = _charService->GetCharacterTalents(characterGuid, spec);
    if (spec < _primaryTalentTree.size())
      _primaryTalentTree[spec] =
          _charService->GetPrimaryTalentTree(characterGuid, spec);
  }
  RecalculateTalentPoints();
}

// Validates and applies a single talent learn for the active spec (client
// 4.3.4 rules). Returns true when a rank was actually learned. The caller is
// responsible for sending SMSG_TALENTS_INFO afterwards.
bool WorldSession::LearnTalent(uint32 talentId, uint32 requestedRank) {
  if (_playerGuid == 0)
    return false;

  auto store = WorldService::Instance().GetTalentStore();
  if (!store) {
    LOG_DEBUG("[TALENT] learn rejected: store not loaded (talent {})", talentId);
    return false;
  }
  if (talentId == 0 || requestedRank >= kMaxTalentRank) {
    LOG_DEBUG("[TALENT] learn rejected: bad args id={} rank={}", talentId,
              requestedRank);
    return false;
  }
  if (_talentFreePoints == 0) {
    LOG_DEBUG("[TALENT] learn rejected: no free points (id={} rank={})", talentId,
              requestedRank);
    return false;
  }

  TalentEntry const *talent = store->GetTalent(talentId);
  if (!talent) {
    LOG_DEBUG("[TALENT] learn rejected: talent {} not in Talent.dbc", talentId);
    return false;
  }
  TalentTabEntry const *tab = store->GetTalentTab(talent->tabId);
  if (!tab) {
    LOG_DEBUG("[TALENT] learn rejected: tab {} not in TalentTab.dbc", talent->tabId);
    return false;
  }

  // Reject talents that belong to another class's tree (anti-cheat).
  uint32 const classMask = (_playerClass > 0) ? (1u << (_playerClass - 1)) : 0u;
  if ((classMask & tab->classMask) == 0u) {
    LOG_DEBUG("[TALENT] learn rejected: class mask (playerClass={} mask={:#x} "
              "tabMask={:#x} id={})",
              _playerClass, classMask, tab->classMask, talentId);
    return false;
  }

  // Highest rank of this talent already known (0 = none).
  uint8 curMaxRank = 0;
  for (int8 rank = static_cast<int8>(kMaxTalentRank) - 1; rank >= 0; --rank) {
    uint32 const rankSpell = talent->spellRank[rank];
    if (rankSpell != 0 && _knownSpellIds.count(rankSpell) != 0u) {
      curMaxRank = static_cast<uint8>(rank + 1);
      break;
    }
  }

  // Already at or above the requested rank.
  if (curMaxRank >= requestedRank + 1)
    return false;
  // Not enough points to bridge from the current rank to the requested one.
  if (_talentFreePoints < (requestedRank - curMaxRank + 1))
    return false;

  // Prerequisite talent (must already have at least PrereqRank in it).
  if (talent->prereqTalent[0] > 0) {
    if (TalentEntry const *dep = store->GetTalent(talent->prereqTalent[0])) {
      bool hasEnoughRank = false;
      for (uint8 r = static_cast<uint8>(talent->prereqRank[0]);
           r < kMaxTalentRank; ++r) {
        uint32 const depSpell = dep->spellRank[r];
        if (depSpell != 0 && _knownSpellIds.count(depSpell) != 0u) {
          hasEnoughRank = true;
          break;
        }
      }
      if (!hasEnoughRank)
        return false;
    }
  }

  // Talents above the first tier require enough points already spent in the
  // tree (5 per tier below). Tally points spent in this talent's tree.
  if (talent->tierId > 0) {
    uint32 spentInTree = 0;
    for (auto const &[id, other] : store->AllTalents()) {
      (void)id;
      if (other.tabId != talent->tabId)
        continue;
      for (uint8 r = 0; r < kMaxTalentRank; ++r) {
        uint32 const sp = other.spellRank[r];
        if (sp != 0 && _knownSpellIds.count(sp) != 0u) {
          spentInTree += static_cast<uint32>(r) + 1u;
          break; // only the highest known rank counts
        }
      }
    }
    if (spentInTree < talent->tierId * kMaxTalentRank) {
      LOG_DEBUG("[TALENT] learn rejected: tier gate (id={} tier={} spentInTree={} "
                "need={})",
                talentId, talent->tierId, spentInTree,
                talent->tierId * kMaxTalentRank);
      return false;
    }
  }

  uint32 const spellId = talent->spellRank[requestedRank];
  if (spellId == 0)
    return false;
  if (_knownSpellIds.count(spellId) != 0u) {
    LOG_DEBUG("[TALENT] learn rejected: spell {} already known (id={} rank={})",
              spellId, talentId, requestedRank);
    return false;
  }
  // The talent's spell is authoritative (it comes from Talent.dbc), so teach it
  // even if the server's spell store doesn't have a row for it — the client
  // knows it from its own Spell.dbc. Just note the gap.
  if (_spellDefinitions && !_spellDefinitions->HasSpell(spellId))
    LOG_DEBUG("[TALENT] spell {} not in server Spell.dbc; teaching anyway "
              "(talent {} rank {})",
              spellId, talentId, requestedRank);

  uint32 const lowGuid = static_cast<uint32>(_playerGuid);

  // Drop any lower-rank spell of this talent: the higher rank supersedes it, and
  // leaving it known would double-count spent points on the next calculation.
  for (uint8 r = 0; r < requestedRank; ++r) {
    uint32 const lowerSpell = talent->spellRank[r];
    if (lowerSpell == 0 || _knownSpellIds.count(lowerSpell) == 0u)
      continue;
    _charService->RemoveCharacterSpell(lowGuid, lowerSpell);
    _knownSpellIds.erase(lowerSpell);
    _knownSpells.erase(
        std::remove(_knownSpells.begin(), _knownSpells.end(), lowerSpell),
        _knownSpells.end());
  }

  // Teach the new rank's spell and persist the talent row.
  if (!_charService->AddCharacterSpell(lowGuid, spellId)) {
    LOG_WARN("LearnTalent: failed to persist spell {} (talent {} rank {})",
             spellId, talentId, requestedRank);
    return false;
  }
  _knownSpells.push_back(spellId);
  _knownSpellIds.insert(spellId);
  SendLearnedSpell(spellId);

  _charService->AddOrUpdateCharacterTalent(
      lowGuid, talentId, static_cast<uint8>(requestedRank),
      _activeActionBarSpec);

  // Reflect the new rank in the in-memory talent list.
  auto it = std::find_if(_characterTalents.begin(), _characterTalents.end(),
                         [&](CharacterTalentRow const &row) {
                           return row.talentId == talentId;
                         });
  if (it != _characterTalents.end()) {
    it->rank = static_cast<uint8>(requestedRank);
  } else {
    CharacterTalentRow row;
    row.talentId = talentId;
    row.rank = static_cast<uint8>(requestedRank);
    row.spec = _activeActionBarSpec;
    _characterTalents.push_back(row);
  }

  // Re-derive free points from level minus the (now updated) points spent.
  RecalculateTalentPoints();
  LOG_DEBUG("[TALENT] learned: talent={} rank={} spell={} freeLeft={}", talentId,
            requestedRank, spellId, _talentFreePoints);
  return true;
}

bool WorldSession::LearnPrimarySpecialization(uint8 tabIndex) {
  if (_playerGuid == 0)
    return false;
  auto store = WorldService::Instance().GetTalentStore();
  if (!store)
    return false;

  uint8 const spec = _activeActionBarSpec;
  if (spec >= _primaryTalentTree.size())
    return false;
  if (_primaryTalentTree[spec] != 0) // already specialized (respec is separate)
    return false;

  std::vector<uint32> const tabs = store->GetClassTalentTabs(_playerClass);
  if (tabIndex >= tabs.size()) {
    LOG_DEBUG("[TALENT] spec rejected: tabIndex {} out of {} tabs (class {})",
              tabIndex, tabs.size(), _playerClass);
    return false;
  }
  uint32 const tabId = tabs[tabIndex];
  TalentTabEntry const *tab = store->GetTalentTab(tabId);
  if (!tab)
    return false;

  uint32 const lowGuid = static_cast<uint32>(_playerGuid);
  _primaryTalentTree[spec] = tabId;
  _charService->SetPrimaryTalentTree(lowGuid, tabId, spec);

  auto teach = [&](uint32 sid) {
    if (sid == 0 || _knownSpellIds.count(sid) != 0u)
      return;
    // Specialization spells come from the DBC and are authoritative; teach even
    // if the server spell store lacks a row (the client has it in Spell.dbc).
    if (_charService->AddCharacterSpell(lowGuid, sid)) {
      _knownSpells.push_back(sid);
      _knownSpellIds.insert(sid);
      SendLearnedSpell(sid);
    }
  };

  // Signature spells (e.g. Mortal Strike for Arms).
  if (std::vector<uint32> const *spells = store->GetPrimaryTreeSpells(tabId))
    for (uint32 sid : *spells)
      teach(sid);

  // Mastery is a level-80 passive; only grant it once the character qualifies.
  if (_playerLevel >= 80)
    for (uint8 i = 0; i < 2; ++i)
      teach(tab->masterySpell[i]);

  LOG_DEBUG("[TALENT] specialization set: spec={} tabIndex={} tabId={}", spec,
            tabIndex, tabId);
  return true;
}

// CMSG_LEARN_TALENT (0x0306): single talent. Payload: uint32 talentId, rank.
void WorldSession::HandleLearnTalent(WorldPacket &packet) {
  if (_playerGuid == 0 || packet.Size() < 8)
    return;
  uint32 const talentId = packet.Read<uint32>();
  uint32 const requestedRank = packet.Read<uint32>();
  if (LearnTalent(talentId, requestedRank))
    SendTalentsInfo();
}

// CMSG_LEARN_PREVIEW_TALENTS (0x2415): batch commit from the talent preview UI.
// Payload: int32 primaryTree, uint32 count, count × {uint32 talentId, uint32 rank}.
void WorldSession::HandleLearnPreviewTalents(WorldPacket &packet) {
  if (_playerGuid == 0 || packet.Size() < 8)
    return;

  // Primary-tree specialization is not tracked yet; read and ignore it.
  uint32 const primaryTree = packet.Read<uint32>(); // int32 primaryTree
  uint32 count = packet.Read<uint32>();

  // Each entry is 8 bytes; clamp the declared count to what the packet holds.
  uint32 const maxEntries = (packet.Size() - 8u) / 8u;
  if (count > maxEntries)
    count = maxEntries;
  LOG_DEBUG("[TALENT] preview commit: size={} primaryTree={} count={} free={}",
            packet.Size(), static_cast<int32>(primaryTree), count,
            _talentFreePoints);

  // A non-negative primaryTree means the player is choosing/confirming the
  // active spec's specialization (the class tab index 0/1/2).
  if (static_cast<int32>(primaryTree) >= 0)
    LearnPrimarySpecialization(static_cast<uint8>(primaryTree));

  bool learnedAny = false;
  for (uint32 i = 0; i < count; ++i) {
    uint32 const talentId = packet.Read<uint32>();
    uint32 const rank = packet.Read<uint32>();
    if (LearnTalent(talentId, rank))
      learnedAny = true;
  }
  (void)learnedAny;
  SendTalentsInfo();
}

// ── Dual specialization (talent groups) ──────────────────────────────────────

// Spellbook ids granted by a talent group: each talent's learned-rank spell plus
// the group's chosen specialization signature spells and (level 80+) mastery.
// Used to swap the spellbook when the active group changes.
std::vector<uint32>
WorldSession::CollectTalentGroupSpells(std::vector<CharacterTalentRow> const &talents,
                                       uint32 primaryTree) const {
  std::vector<uint32> out;
  auto store = WorldService::Instance().GetTalentStore();
  if (!store)
    return out;

  for (CharacterTalentRow const &t : talents) {
    if (t.rank >= kMaxTalentRank)
      continue;
    if (TalentEntry const *te = store->GetTalent(t.talentId)) {
      uint32 const sp = te->spellRank[t.rank];
      if (sp != 0)
        out.push_back(sp);
    }
  }

  if (primaryTree != 0) {
    if (std::vector<uint32> const *sig = store->GetPrimaryTreeSpells(primaryTree))
      out.insert(out.end(), sig->begin(), sig->end());
    if (_playerLevel >= 80) {
      if (TalentTabEntry const *tab = store->GetTalentTab(primaryTree))
        for (uint8 i = 0; i < 2; ++i)
          if (tab->masterySpell[i] != 0)
            out.push_back(tab->masterySpell[i]);
    }
  }
  return out;
}

// Switches the active talent group (dual spec). Unlearns the leaving group's
// talent/spec spells, swaps in the entering group's persisted talents, glyphs and
// action bars, and pushes the refreshed state to the client.
//
// NOTE (staged): 4.3.4 (build 15595) has NO dedicated activate-spec opcode — the
// reference Cataclysm core leaves the spec opcodes (set-primary-tree,
// unlearn-specialization) unhandled — so this has no client trigger yet. It is the
// complete switch mechanism, awaiting a trigger (a GM command, or routing through
// the handled CMSG_LEARN_PREVIEW_TALENTS path). Persistence of the active group
// across logins is also pending (needs a `characters` column); a relog resets to
// group 0.
bool WorldSession::ActivateTalentGroup(uint8 group) {
  if (_playerGuid == 0)
    return false;
  if (group >= ActionButton::kMaxActionBarSpecs) {
    LOG_DEBUG("[TALENT] activate rejected: group {} out of range", group);
    return false;
  }
  if (group == _activeActionBarSpec)
    return false; // already active

  uint32 const lowGuid = static_cast<uint32>(_playerGuid);

  // 1) Unlearn the spells the leaving group granted (talent ranks + spec spells).
  uint32 const oldTree = (_activeActionBarSpec < _primaryTalentTree.size())
                             ? _primaryTalentTree[_activeActionBarSpec]
                             : 0u;
  std::vector<uint32> removed;
  for (uint32 const sp : CollectTalentGroupSpells(_characterTalents, oldTree)) {
    if (sp == 0 || _knownSpellIds.count(sp) == 0u)
      continue;
    _charService->RemoveCharacterSpell(lowGuid, sp);
    _knownSpellIds.erase(sp);
    _knownSpells.erase(std::remove(_knownSpells.begin(), _knownSpells.end(), sp),
                       _knownSpells.end());
    removed.push_back(sp);
  }
  if (!removed.empty())
    SendUnlearnSpells(removed);

  // 2) Switch group and reload its persisted talents / glyphs / action bars.
  //    LoadGlyphsForCharacter strips the leaving group's glyph auras (its socket
  //    table is still in place at this point) before loading the new layout.
  _activeActionBarSpec = group;
  LoadTalentsForCharacter(lowGuid);
  LoadGlyphsForCharacter(lowGuid);
  LoadActionButtonsForCharacter(lowGuid);

  // 3) Learn the entering group's spells.
  uint32 const newTree = (_activeActionBarSpec < _primaryTalentTree.size())
                             ? _primaryTalentTree[_activeActionBarSpec]
                             : 0u;
  for (uint32 const sp : CollectTalentGroupSpells(_characterTalents, newTree)) {
    if (sp == 0 || _knownSpellIds.count(sp) != 0u)
      continue;
    if (_charService->AddCharacterSpell(lowGuid, sp)) {
      _knownSpells.push_back(sp);
      _knownSpellIds.insert(sp);
      SendLearnedSpell(sp);
    }
  }

  // 4) Apply the entering group's glyph effect auras and refresh the client.
  ApplySocketedGlyphAuras();
  RecalculateTalentPoints();
  SendGlyphSlotFields();
  SendTalentsInfo();
  SendActionButtons(/*reason=*/2); // 2 = updated (vs 0 = initial login burst)
  LOG_INFO("[TALENT] active group switched to {} (guid {})", group, lowGuid);
  return true;
}

// ── Glyphs ──────────────────────────────────────────────────────────────────

namespace {
// Minimum level that unlocks each glyph slot index (4.3.4).
uint8 GlyphSlotMinLevel(uint8 slotIndex) {
  switch (slotIndex) {
  case 0:
  case 1:
  case 6:
    return 25;
  case 2:
  case 3:
  case 7:
    return 50;
  case 4:
  case 5:
  case 8:
    return 75;
  default:
    return 0;
  }
}
} // namespace

void WorldSession::LoadGlyphsForCharacter(uint32 characterGuid) {
  // Strip the effect auras of the spec we are leaving before swapping the socket
  // table. On a spec switch the caller reloads the new spec's glyphs here and then
  // calls ApplySocketedGlyphAuras(); at login `_glyphs` is empty so this no-ops.
  RemoveSocketedGlyphAuras();
  _glyphs = {};
  if (_charService && characterGuid != 0u) {
    for (CharacterGlyphRow const &r :
         _charService->GetCharacterGlyphs(characterGuid, _activeActionBarSpec)) {
      if (r.slot < _glyphs.size())
        _glyphs[r.slot] = r.glyph;
    }
  }
}

void WorldSession::SendGlyphSlotFields() {
  if (_playerGuid == 0)
    return;
  std::map<uint16, uint32> f;

  // Which GlyphSlot.dbc entry each of the 9 sockets is (DBC row order).
  if (auto store = WorldService::Instance().GetTalentStore()) {
    std::vector<uint32> const &slots = store->GlyphSlotIds();
    for (uint8 i = 0; i < kMaxGlyphSlots && i < slots.size(); ++i)
      f[static_cast<uint16>(PLAYER_FIELD_GLYPH_SLOTS_1 + i)] = slots[i];
  }

  // Enabled-slot bitmask by level: 3 at 25, +3 at 50, +3 at 75.
  uint32 mask = 0;
  if (_playerLevel >= 25)
    mask |= 0x01u | 0x02u | 0x40u;
  if (_playerLevel >= 50)
    mask |= 0x04u | 0x08u | 0x80u;
  if (_playerLevel >= 75)
    mask |= 0x10u | 0x20u | 0x100u;
  f[PLAYER_GLYPHS_ENABLED] = mask;

  for (uint8 i = 0; i < kMaxGlyphSlots; ++i)
    f[static_cast<uint16>(PLAYER_FIELD_GLYPHS_1 + i)] = _glyphs[i];

  UpdateData update(_mapId);
  update.AddValuesUpdate(_playerGuid, f);
  WorldPacket pkt(SMSG_UPDATE_OBJECT);
  update.Build(pkt);
  SendPacket(pkt);
}

bool WorldSession::ApplyGlyph(uint8 slotIndex, uint32 glyphId) {
  if (_playerGuid == 0 || slotIndex >= kMaxGlyphSlots)
    return false;
  auto store = WorldService::Instance().GetTalentStore();
  if (!store)
    return false;

  uint8 const minLevel = GlyphSlotMinLevel(slotIndex);
  if (minLevel != 0 && _playerLevel < minLevel) {
    LOG_DEBUG("[GLYPH] rejected: slot {} locked (level {} < {})", slotIndex,
              _playerLevel, minLevel);
    return false;
  }

  if (glyphId != 0) {
    GlyphPropertiesEntry const *props = store->GetGlyphProperties(glyphId);
    if (!props) {
      LOG_DEBUG("[GLYPH] rejected: glyph {} not in GlyphProperties.dbc", glyphId);
      return false;
    }
    // The glyph's type must match the socket's type.
    std::vector<uint32> const &slots = store->GlyphSlotIds();
    if (slotIndex < slots.size()) {
      uint32 const slotType = store->GetGlyphSlotType(slots[slotIndex]);
      if (slotType != UINT32_MAX && props->type != slotType) {
        LOG_DEBUG("[GLYPH] rejected: type mismatch glyph {} (type {}) vs slot {} "
                  "(type {})",
                  glyphId, props->type, slotIndex, slotType);
        return false;
      }
    }
  }

  // Drop the effect spell of whatever glyph currently occupies the slot, then
  // grant the new one's. The socket field (sent below) is only the UI side; the
  // gameplay effect comes from the GlyphProperties spell applied as an aura.
  uint32 const prevGlyphId = _glyphs[slotIndex];
  if (prevGlyphId != 0 && prevGlyphId != glyphId) {
    if (GlyphPropertiesEntry const *prev = store->GetGlyphProperties(prevGlyphId))
      ApplyGlyphSpellAura(prev->spellId, /*apply=*/false);
  }

  _glyphs[slotIndex] = glyphId;
  _charService->SetCharacterGlyph(static_cast<uint32>(_playerGuid), slotIndex,
                                  glyphId, _activeActionBarSpec);

  if (glyphId != 0) {
    if (GlyphPropertiesEntry const *props = store->GetGlyphProperties(glyphId))
      ApplyGlyphSpellAura(props->spellId, /*apply=*/true);
  }

  SendGlyphSlotFields();
  SendTalentsInfo();
  LOG_DEBUG("[GLYPH] applied: slot {} glyph {}", slotIndex, glyphId);
  return true;
}

// Casts (apply) or strips (remove) the passive aura that a glyph's effect spell
// grants. The effect spell lives in GlyphProperties.dbc; without applying it the
// glyph would show as socketed but do nothing in-game.
void WorldSession::ApplyGlyphSpellAura(uint32 glyphSpellId, bool apply) {
  if (_playerGuid == 0 || glyphSpellId == 0)
    return;
  auto map = runtime().GetMap(_mapId);
  if (!map)
    return;
  uint8 const level = _playerLevel > 0 ? _playerLevel : 1u;

  if (!apply) {
    RemovePlayerAuraOnMap(_mapId, map, _playerGuid, glyphSpellId, level);
    return;
  }

  if (auto pl = map->TryGetPlayer(_playerGuid))
    if (pl->HasAura(glyphSpellId))
      return; // already active (e.g. re-applied on login)

  auto defs = runtime().GetSpellDefinitions();
  if (!defs)
    return;
  std::optional<SpellDefinition> def = defs->GetDefinition(glyphSpellId);
  // The glyph's effect spell may be absent from the server Spell.dbc (custom or
  // cross-expansion content). Nothing to apply then — the socket field stands.
  if (!def) {
    LOG_DEBUG("[GLYPH] effect spell {} not in server Spell.dbc; socket only",
              glyphSpellId);
    return;
  }

  auto tables = runtime().GetSpellCastTables();
  auto const now = std::chrono::steady_clock::now();
  SpellCastOutcome out{};
  SpellHitEffects::ApplyAuraFromDefinition(&*def, _playerGuid, _playerGuid, level,
                                           now, tables.get(), &out);
  if (out.hasAuraApply)
    ApplySpellCastAuraOnMap(_mapId, map, out, now);
}

void WorldSession::ApplySocketedGlyphAuras() {
  auto store = WorldService::Instance().GetTalentStore();
  if (!store)
    return;
  for (uint8 i = 0; i < kMaxGlyphSlots; ++i) {
    uint32 const glyphId = _glyphs[i];
    if (glyphId == 0)
      continue;
    if (GlyphPropertiesEntry const *props = store->GetGlyphProperties(glyphId))
      ApplyGlyphSpellAura(props->spellId, /*apply=*/true);
  }
}

void WorldSession::RemoveSocketedGlyphAuras() {
  auto store = WorldService::Instance().GetTalentStore();
  if (!store)
    return;
  for (uint8 i = 0; i < kMaxGlyphSlots; ++i) {
    uint32 const glyphId = _glyphs[i];
    if (glyphId == 0)
      continue;
    if (GlyphPropertiesEntry const *props = store->GetGlyphProperties(glyphId))
      ApplyGlyphSpellAura(props->spellId, /*apply=*/false);
  }
}

} // namespace Firelands
