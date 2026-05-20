#pragma once

#include <shared/Common.h>

#include <chrono>
#include <cstdint>
#include <vector>

namespace Firelands {

/// Client `SMSG_AURA_UPDATE` fields captured at apply (for expire/remove wire).
struct AuraClientWireMeta {
  uint8 effectIndex = 0;
  uint8 activeEffectMask = 0;
  uint8 casterLevel = 1;
  uint64 casterGuid = 0;
  uint32 maxDurationMs = 0;
  int32 effectAmount = 0;
  bool sendEffectAmount = false;
  bool isNegative = false;
};

struct AuraRemoval {
  uint32 spellId = 0;
  uint8 visualSlot = 0;
  AuraClientWireMeta wire{};
};

struct AuraPeriodicTick {
  uint32 spellId = 0;
  int32 healthDelta = 0;
  uint64 casterGuid = 0;
  uint32 auraEffectType = 0;
  uint8 visualSlot = 0;
  uint32 remainingMs = 0;
  AuraClientWireMeta wire{};
};

/// Result of one aura tick pass: expiry removals first, then periodic effects.
struct UnitAuraTickResult {
  std::vector<AuraRemoval> removals;
  std::vector<AuraPeriodicTick> periodicTicks;
};

class Aura {
public:
  Aura(std::uint32_t spellId, std::uint32_t auraEffectType, std::int32_t basePoints,
       std::int32_t dieSides, std::uint64_t casterGuid,
       std::chrono::steady_clock::time_point expireTime, std::uint8_t visualSlot,
       std::uint32_t periodicPeriodMs, std::int32_t periodicHealthDeltaPerTick,
       std::chrono::steady_clock::time_point nextPeriodicTick,
       AuraClientWireMeta wireMeta = {})
      : _spellId(spellId), _auraEffectType(auraEffectType), _basePoints(basePoints),
        _dieSides(dieSides), _casterGuid(casterGuid), _expireTime(expireTime),
        _visualSlot(visualSlot), _periodicPeriodMs(periodicPeriodMs),
        _periodicHealthDeltaPerTick(periodicHealthDeltaPerTick),
        _nextPeriodicTick(nextPeriodicTick), _wireMeta(wireMeta) {}

  std::uint32_t GetSpellId() const { return _spellId; }
  std::uint32_t GetAuraEffectType() const { return _auraEffectType; }
  std::int32_t GetBasePoints() const { return _basePoints; }
  std::int32_t GetDieSides() const { return _dieSides; }
  std::uint64_t GetCasterGuid() const { return _casterGuid; }
  std::chrono::steady_clock::time_point GetExpireTime() const { return _expireTime; }
  std::uint8_t GetVisualSlot() const { return _visualSlot; }
  std::uint32_t GetPeriodicPeriodMs() const { return _periodicPeriodMs; }
  std::int32_t GetPeriodicHealthDeltaPerTick() const {
    return _periodicHealthDeltaPerTick;
  }

  bool IsExpired(std::chrono::steady_clock::time_point now) const {
    return now >= _expireTime;
  }

  /// Milliseconds until expiry; 0 when expired or at the expiry instant.
  std::chrono::milliseconds GetRemainingMs(
      std::chrono::steady_clock::time_point now) const {
    if (now >= _expireTime)
      return std::chrono::milliseconds{0};
    return std::chrono::duration_cast<std::chrono::milliseconds>(_expireTime - now);
  }

  bool IsPeriodic() const {
    return _periodicPeriodMs > 0u && _periodicHealthDeltaPerTick != 0;
  }

  bool IsPeriodicDue(std::chrono::steady_clock::time_point now) const {
    if (!IsPeriodic() || IsExpired(now))
      return false;
    return now >= _nextPeriodicTick;
  }

  void AdvancePeriodicTick(std::chrono::steady_clock::time_point now) {
    _nextPeriodicTick = now + std::chrono::milliseconds(_periodicPeriodMs);
  }

  std::int32_t GetMagnitude() const {
    if (_dieSides == 0)
      return _basePoints;
    return _basePoints + (_dieSides > 0 ? (_dieSides / 2) : 0);
  }

  AuraClientWireMeta const &GetClientWireMeta() const { return _wireMeta; }

private:
  std::uint32_t _spellId;
  std::uint32_t _auraEffectType;
  std::int32_t _basePoints;
  std::int32_t _dieSides;
  std::uint64_t _casterGuid;
  std::chrono::steady_clock::time_point _expireTime;
  std::uint8_t _visualSlot;
  std::uint32_t _periodicPeriodMs;
  std::int32_t _periodicHealthDeltaPerTick;
  std::chrono::steady_clock::time_point _nextPeriodicTick;
  AuraClientWireMeta _wireMeta;
};

} // namespace Firelands
