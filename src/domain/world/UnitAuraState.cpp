#include <domain/world/UnitAuraState.h>

namespace Firelands {

uint8 UnitAuraState::AllocateAuraVisualSlot(uint32 spellId) {
  auto const it = m_auras.find(spellId);
  if (it != m_auras.end())
    return it->second.GetVisualSlot();
  uint8 const slot = m_nextAuraSlot % 40;
  ++m_nextAuraSlot;
  return slot;
}

void UnitAuraState::AddAura(Aura const &aura) {
  m_auras.insert_or_assign(aura.GetSpellId(), aura);
}

void UnitAuraState::RemoveAura(uint32 spellId) { (void)TryRemoveAura(spellId); }

std::optional<AuraRemoval> UnitAuraState::TryRemoveAura(uint32 spellId) {
  auto it = m_auras.find(spellId);
  if (it == m_auras.end())
    return std::nullopt;
  AuraRemoval const removal{it->first, it->second.GetVisualSlot()};
  m_auras.erase(it);
  return removal;
}

bool UnitAuraState::HasAura(uint32 spellId) const {
  return m_auras.find(spellId) != m_auras.end();
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
      removed.push_back(AuraRemoval{it->first, it->second.GetVisualSlot()});
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
    if (!aura.IsPeriodicDue(now))
      continue;
    ticks.push_back(
        AuraPeriodicTick{aura.GetSpellId(), aura.GetPeriodicHealthDeltaPerTick()});
    aura.AdvancePeriodicTick(now);
  }
  return ticks;
}

} // namespace Firelands
