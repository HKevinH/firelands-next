#include <domain/world/UnitAuraState.h>

namespace Firelands {

uint8 UnitAuraState::AllocateAuraVisualSlot(uint32 spellId) {
  auto const it = m_auras.find(spellId);
  if (it != m_auras.end())
    return it->second.GetVisualSlot();
  uint8 const slot = m_nextAuraSlot % 64;
  ++m_nextAuraSlot;
  return slot;
}

void UnitAuraState::AddAura(Aura const &aura) {
  m_auras.insert_or_assign(aura.GetSpellId(), aura);
}

void UnitAuraState::RemoveAura(uint32 spellId) { (void)TryRemoveAura(spellId); }

std::optional<AuraRemoval> UnitAuraState::TryRemoveAura(uint32 spellId,
                                                        uint64 casterGuid) {
  auto it = m_auras.find(spellId);
  if (it == m_auras.end())
    return std::nullopt;
  if (casterGuid != 0u && it->second.GetCasterGuid() != casterGuid)
    return std::nullopt;
  AuraRemoval removal{};
  removal.spellId = it->first;
  removal.visualSlot = it->second.GetVisualSlot();
  removal.wire = it->second.GetClientWireMeta();
  m_auras.erase(it);
  return removal;
}

bool UnitAuraState::HasAura(uint32 spellId) const {
  auto const it = m_auras.find(spellId);
  if (it == m_auras.end())
    return false;
  return !it->second.IsExpired(std::chrono::steady_clock::now());
}

std::vector<Aura> UnitAuraState::GetActiveAuras() const {
  auto const now = std::chrono::steady_clock::now();
  std::vector<Aura> activeAuras;
  activeAuras.reserve(m_auras.size());
  for (auto const &pair : m_auras) {
    if (!pair.second.IsExpired(now))
      activeAuras.push_back(pair.second);
  }
  return activeAuras;
}

std::vector<AuraRemoval> UnitAuraState::UpdateAuras(
    std::chrono::steady_clock::time_point now) {
  std::vector<AuraRemoval> removed;
  for (auto it = m_auras.begin(); it != m_auras.end();) {
    if (it->second.IsExpired(now)) {
      AuraRemoval removal{};
      removal.spellId = it->first;
      removal.visualSlot = it->second.GetVisualSlot();
      removal.wire = it->second.GetClientWireMeta();
      removed.push_back(removal);
      it = m_auras.erase(it);
    } else {
      ++it;
    }
  }
  return removed;
}

std::vector<AuraPeriodicTick> UnitAuraState::TickPeriodicAuras(
    std::chrono::steady_clock::time_point now) {
  std::vector<AuraPeriodicTick> ticks;
  for (auto &pair : m_auras) {
    Aura &aura = pair.second;
    if (aura.IsExpired(now))
      continue;
    if (!aura.IsPeriodicDue(now))
      continue;
    AuraPeriodicTick tick{};
    tick.spellId = aura.GetSpellId();
    tick.healthDelta = aura.GetPeriodicHealthDeltaPerTick();
    tick.casterGuid = aura.GetCasterGuid();
    tick.auraEffectType = aura.GetAuraEffectType();
    tick.visualSlot = aura.GetVisualSlot();
    tick.remainingMs =
        static_cast<uint32>(aura.GetRemainingMs(now).count());
    tick.wire = aura.GetClientWireMeta();
    ticks.push_back(tick);
    aura.AdvancePeriodicTick(now);
  }
  return ticks;
}

UnitAuraTickResult UnitAuraState::Tick(std::chrono::steady_clock::time_point now) {
  UnitAuraTickResult result;
  result.removals = UpdateAuras(now);
  result.periodicTicks = TickPeriodicAuras(now);
  return result;
}

} // namespace Firelands
