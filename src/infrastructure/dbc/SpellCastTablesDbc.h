#pragma once

#include <domain/repositories/ISpellCastTables.h>
#include <string>
#include <unordered_map>

namespace Firelands {

/// Loads `SpellCastTimes.dbc`, `SpellRange.dbc`, and optionally `SpellCooldowns.dbc`
/// (Cataclysm 4.3.4 / TCPP `DBCfmt.h`).
class SpellCastTablesDbc final : public ISpellCastTables {
public:
  struct CooldownRow {
    uint32 categoryRecoveryMs = 0;
    uint32 recoveryMs = 0;
    uint32 startRecoveryMs = 0;
  };

  /// Each file is optional: missing file logs a warning and that table stays empty.
  bool Load(std::string const &spellCastTimesPath, std::string const &spellRangePath,
            std::string const &spellCooldownsPath);

  bool HasCastTimes() const { return !m_castBaseMs.empty(); }
  bool HasRanges() const { return !m_rangeMaxYards.empty(); }
  bool HasCooldowns() const { return !m_cooldowns.empty(); }

  uint32 GetCastTimeMs(uint32 castingTimeIndex) const override;
  float GetHostileRangeMaxYards(uint32 rangeIndex) const override;
  void GetCooldownTiming(uint32 cooldownsId, uint32 *categoryRecoveryMs,
                         uint32 *recoveryMs, uint32 *startRecoveryMs) const override;

private:
  std::unordered_map<uint32, int32> m_castBaseMs;
  std::unordered_map<uint32, float> m_rangeMaxYards;
  std::unordered_map<uint32, CooldownRow> m_cooldowns;
};

} // namespace Firelands
