#include <application/services/PlayerCreateInfoService.h>
#include <application/spell/SpellManager.h>
#include <domain/models/CharacterCooldown.h>
#include <infrastructure/network/sessions/WorldSession.h>
#include <shared/Logger.h>
#include <shared/network/SpellCooldownWire.h>
#include <shared/network/WorldPacket.h>

#include <unordered_set>
#include <vector>

namespace Firelands {

namespace {

void SendCooldownEventPacket(WorldSession &session, uint32 spellId) {
  if (spellId == 0)
    return;
  WorldPacket pkt;
  SpellCooldownWire::BuildCooldownEvent(pkt, static_cast<int32>(spellId), false);
  session.SendPacket(pkt);
}

void SendSpellCooldownRows(WorldSession &session, uint64 unitGuid, uint8 flags,
                           std::vector<SpellCooldownWire::SpellCooldownStruct> const &rows) {
  if (unitGuid == 0 || rows.empty())
    return;
  WorldPacket pkt;
  SpellCooldownWire::BuildSpellCooldown(pkt, unitGuid, flags, rows);
  session.SendPacket(pkt);
}

uint32 GcdRemainingMs(std::chrono::steady_clock::time_point gcdReady,
                      std::chrono::steady_clock::time_point now) {
  if (gcdReady <= now)
    return 0;
  auto const rem = std::chrono::duration_cast<std::chrono::milliseconds>(gcdReady - now);
  return rem.count() > 0 ? static_cast<uint32>(rem.count()) : 0u;
}

std::vector<SpellCooldownWire::CategoryCooldownEntry> BuildActiveCategoryCooldownEntries(
    std::unordered_map<uint32, std::chrono::steady_clock::time_point> const &untilByCategory,
    std::chrono::steady_clock::time_point now) {
  std::vector<SpellCooldownWire::CategoryCooldownEntry> entries;
  entries.reserve(untilByCategory.size());
  for (auto const &[category, until] : untilByCategory) {
    if (until <= now || category == 0u)
      continue;
    auto const rem =
        std::chrono::duration_cast<std::chrono::milliseconds>(until - now).count();
    if (rem <= 0)
      continue;
    entries.push_back(SpellCooldownWire::CategoryCooldownEntry{
        static_cast<int32>(category), static_cast<int32>(rem)});
  }
  return entries;
}

} // namespace

void WorldSession::SendClientSpellCooldownsAfterCast(
    uint32 spellId, uint32 spellCooldownDurationMs,
    std::chrono::steady_clock::time_point gcdReady,
    std::chrono::steady_clock::time_point now) {
  if (_playerGuid == 0 || spellId == 0)
    return;

  // Cataclysm 4.3.4: CooldownEvent drives action-bar swipe; SpellCooldown carries durations.
  SendCooldownEventPacket(*this, spellId);

  uint32 const gcdMs = GcdRemainingMs(gcdReady, now);
  if (gcdMs > 0) {
    SpellCooldownWire::SpellCooldownStruct row{spellId, gcdMs, 1.0f};
    SendSpellCooldownRows(*this, _playerGuid, SpellCooldownWire::kIncludeGcd, {row});
  }

  if (spellCooldownDurationMs > 0) {
    SpellCooldownWire::SpellCooldownStruct row{spellId, spellCooldownDurationMs, 1.0f};
    SendSpellCooldownRows(*this, _playerGuid, SpellCooldownWire::kNone, {row});
  }
}

void WorldSession::SendClientActiveSpellCooldowns() {
  if (_playerGuid == 0)
    return;

  auto const now = std::chrono::steady_clock::now();
  std::vector<SpellCooldownWire::SpellCooldownStruct> entries;
  entries.reserve(_spellCooldownUntil.size());
  for (auto const &[spellId, until] : _spellCooldownUntil) {
    if (until <= now)
      continue;
    auto const rem =
        std::chrono::duration_cast<std::chrono::milliseconds>(until - now).count();
    if (rem <= 0)
      continue;
    entries.push_back(
        SpellCooldownWire::SpellCooldownStruct{spellId, static_cast<uint32>(rem), 1.0f});
  }
  if (entries.empty())
    return;

  SendSpellCooldownRows(*this, _playerGuid, SpellCooldownWire::kNone, entries);
}

void WorldSession::SendClientActiveCategoryCooldowns() {
  if (_playerGuid == 0)
    return;

  auto const now = std::chrono::steady_clock::now();
  std::vector<SpellCooldownWire::CategoryCooldownEntry> const entries =
      BuildActiveCategoryCooldownEntries(_spellCategoryCooldownUntil, now);

  WorldPacket pkt;
  SpellCooldownWire::BuildCategoryCooldown(pkt, entries.data(), entries.size());
  SendPacket(pkt);
}

void WorldSession::CommitSpellCooldownsFromCast(uint32 spellId, SpellCastOutcome const &out,
                                                std::chrono::steady_clock::time_point now) {
  if (out.spellCooldownDurationMs > 0u)
    _spellCooldownUntil[spellId] =
        now + std::chrono::milliseconds(static_cast<int64_t>(out.spellCooldownDurationMs));
  if (out.spellCategoryCooldownGroup > 0u && out.spellCategoryCooldownDurationMs > 0u) {
    _spellCategoryCooldownUntil[out.spellCategoryCooldownGroup] =
        now + std::chrono::milliseconds(
                  static_cast<int64_t>(out.spellCategoryCooldownDurationMs));
  }
  SendClientSpellCooldownsAfterCast(spellId, out.spellCooldownDurationMs, out.newGcdReady,
                                    now);
  SendClientActiveCategoryCooldowns();
}

void WorldSession::CommitSpellRecoveryCooldownFromDeferred(
    uint32 spellId, uint32 spellCooldownDurationMs,
    std::chrono::steady_clock::time_point now) {
  if (spellCooldownDurationMs > 0u) {
    _spellCooldownUntil[spellId] =
        now + std::chrono::milliseconds(static_cast<int64_t>(spellCooldownDurationMs));
  }
  SendClientSpellCooldownsAfterCast(spellId, spellCooldownDurationMs, now, now);
  SendClientActiveCategoryCooldowns();
}

void WorldSession::HandleRequestCategoryCooldowns(WorldPacket & /*packet*/) {
  SendClientActiveCategoryCooldowns();
}

void WorldSession::RestorePersistedSpellCooldowns(uint32 characterGuid) {
  if (characterGuid == 0 || !_charService)
    return;

  CharacterCooldownState const state =
      _charService->LoadCharacterCooldowns(characterGuid);
  auto const now = std::chrono::steady_clock::now();

  for (PersistedSpellCooldown const &row : state.spellCooldowns) {
    _spellCooldownUntil[row.spellId] =
        now + std::chrono::milliseconds(static_cast<int64_t>(row.remainingMs));
  }
  for (PersistedCategoryCooldown const &row : state.categoryCooldowns) {
    _spellCategoryCooldownUntil[row.category] =
        now + std::chrono::milliseconds(static_cast<int64_t>(row.remainingMs));
  }
}

bool WorldSession::GmResetAllCooldowns() {
  if (_playerGuid == 0)
    return false;

  auto const now = std::chrono::steady_clock::now();
  bool const hadGcd = _gcdReady > now;
  uint32 const gcdSpellId = _gcdTriggerSpellId;

  std::unordered_set<uint32> spellIdsToClear;
  spellIdsToClear.reserve(_spellCooldownUntil.size() + _knownSpells.size());
  for (auto const &[spellId, until] : _spellCooldownUntil) {
    if (until > now)
      spellIdsToClear.insert(spellId);
  }
  for (uint32 spellId : _knownSpells) {
    if (spellId != 0u)
      spellIdsToClear.insert(spellId);
  }
  if (_charService) {
    if (PlayerCreateInfoService const *pci =
            _charService->GetPlayerCreateInfoService()) {
      for (uint32_t racialId : pci->GetRacialSpells(_playerRace, _playerClass)) {
        if (racialId != 0u)
          spellIdsToClear.insert(racialId);
      }
    }
  }

  std::vector<uint32> clearList(spellIdsToClear.begin(), spellIdsToClear.end());

  _spellCooldownUntil.clear();
  _spellCategoryCooldownUntil.clear();
  _gcdReady = {};
  _gcdTriggerSpellId = 0;

  if (!clearList.empty()) {
    WorldPacket pkt;
    SpellCooldownWire::BuildClearCooldowns(pkt, _playerGuid, clearList.data(),
                                           clearList.size());
    SendPacket(pkt);
  }
  if (hadGcd && gcdSpellId != 0u) {
    SpellCooldownWire::SpellCooldownStruct row{gcdSpellId, 0u, 1.0f};
    SendSpellCooldownRows(*this, _playerGuid, SpellCooldownWire::kIncludeGcd, {row});
  }

  SendClientActiveCategoryCooldowns();

  uint32 const charGuidLow = static_cast<uint32>(_playerGuid);
  if (_charService && charGuidLow != 0u) {
    CharacterCooldownState empty;
    if (!_charService->SaveCharacterCooldowns(charGuidLow, empty))
      LOG_WARN("SaveCharacterCooldowns failed after GM cd reset for guid {}", charGuidLow);
  }

  return true;
}

void WorldSession::SavePersistedSpellCooldowns(uint32 characterGuid) {
  if (characterGuid == 0 || !_charService)
    return;

  auto const now = std::chrono::steady_clock::now();
  CharacterCooldownState state;

  for (auto const &[spellId, until] : _spellCooldownUntil) {
    if (until <= now)
      continue;
    auto const rem =
        std::chrono::duration_cast<std::chrono::milliseconds>(until - now).count();
    if (rem <= 0)
      continue;
    state.spellCooldowns.push_back(
        PersistedSpellCooldown{spellId, static_cast<uint32>(rem)});
  }
  for (auto const &[category, until] : _spellCategoryCooldownUntil) {
    if (until <= now || category == 0u)
      continue;
    auto const rem =
        std::chrono::duration_cast<std::chrono::milliseconds>(until - now).count();
    if (rem <= 0)
      continue;
    state.categoryCooldowns.push_back(
        PersistedCategoryCooldown{category, static_cast<uint32>(rem)});
  }

  if (!_charService->SaveCharacterCooldowns(characterGuid, state))
    LOG_WARN("SaveCharacterCooldowns failed for guid {}", characterGuid);
}

} // namespace Firelands
