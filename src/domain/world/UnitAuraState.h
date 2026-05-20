#pragma once

#include <domain/world/Aura.h>
#include <chrono>
#include <cstdint>
#include <optional>
#include <unordered_map>
#include <vector>

namespace Firelands {

/// Per-unit aura storage (players and creatures).
class UnitAuraState {
public:
  uint8 AllocateAuraVisualSlot(uint32 spellId);
  void AddAura(Aura const &aura);
  void RemoveAura(uint32 spellId);
  std::optional<AuraRemoval> TryRemoveAura(uint32 spellId, uint64 casterGuid = 0);
  bool HasAura(uint32 spellId) const;
  std::vector<Aura> GetActiveAuras() const;
  std::vector<AuraRemoval> UpdateAuras(std::chrono::steady_clock::time_point now);
  std::vector<AuraPeriodicTick> TickPeriodicAuras(
      std::chrono::steady_clock::time_point now);
  /// Removes expired auras, then runs periodic ticks on survivors (single pass).
  UnitAuraTickResult Tick(std::chrono::steady_clock::time_point now);

private:
  std::unordered_map<uint32_t, Aura> m_auras;
  uint8_t m_nextAuraSlot = 0;
};

} // namespace Firelands
