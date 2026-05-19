#include <application/spell/SpellManager.h>
#include <domain/models/CharacterCooldown.h>
#include <infrastructure/network/sessions/WorldSession.h>
#include <shared/Logger.h>
#include <shared/network/SpellCooldownWire.h>
#include <shared/network/WorldPacket.h>

#include <vector>

namespace Firelands {

namespace {

void SendGcdCooldownPacket(WorldSession &session, uint64 unitGuid, uint32 spellId,
                           std::chrono::steady_clock::time_point gcdReady,
                           std::chrono::steady_clock::time_point now) {
  if (unitGuid == 0 || gcdReady <= now)
    return;
  auto const rem = std::chrono::duration_cast<std::chrono::milliseconds>(gcdReady - now);
  if (rem.count() <= 0)
    return;

  SpellCooldownWire::SpellCooldownEntry entry{spellId, 0};
  WorldPacket pkt;
  SpellCooldownWire::BuildSpellCooldown(pkt, unitGuid, SpellCooldownWire::kIncludeGcd,
                                        &entry, 1);
  session.SendPacket(pkt);
}

void SendSpellRecoveryCooldownPacket(WorldSession &session, uint64 unitGuid, uint32 spellId,
                                   uint32 spellCooldownDurationMs) {
  if (unitGuid == 0 || spellCooldownDurationMs == 0u)
    return;

  SpellCooldownWire::SpellCooldownEntry entry{
      spellId, static_cast<int32>(spellCooldownDurationMs)};
  WorldPacket pkt;
  SpellCooldownWire::BuildSpellCooldown(pkt, unitGuid, SpellCooldownWire::kNone, &entry, 1);
  session.SendPacket(pkt);
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
  if (_playerGuid == 0)
    return;
  SendGcdCooldownPacket(*this, _playerGuid, spellId, gcdReady, now);
  SendSpellRecoveryCooldownPacket(*this, _playerGuid, spellId, spellCooldownDurationMs);
}

void WorldSession::SendClientActiveSpellCooldowns() {
  if (_playerGuid == 0)
    return;

  auto const now = std::chrono::steady_clock::now();
  std::vector<SpellCooldownWire::SpellCooldownEntry> entries;
  entries.reserve(_spellCooldownUntil.size());
  for (auto const &[spellId, until] : _spellCooldownUntil) {
    if (until <= now)
      continue;
    auto const rem =
        std::chrono::duration_cast<std::chrono::milliseconds>(until - now).count();
    if (rem <= 0)
      continue;
    entries.push_back(
        SpellCooldownWire::SpellCooldownEntry{spellId, static_cast<int32>(rem)});
  }
  if (entries.empty())
    return;

  WorldPacket pkt;
  SpellCooldownWire::BuildSpellCooldown(pkt, _playerGuid, SpellCooldownWire::kNone,
                                        entries.data(), entries.size());
  SendPacket(pkt);
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
