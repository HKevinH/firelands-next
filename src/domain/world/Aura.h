#pragma once

#include <shared/Common.h>

#include <chrono>
#include <cstdint>

namespace Firelands {

struct AuraRemoval {
  uint32 spellId = 0;
  uint8 visualSlot = 0;
};

struct AuraPeriodicTick {
  uint32 spellId = 0;
  int32 healthDelta = 0;
};

class Aura {
public:
  Aura(std::uint32_t spellId, std::uint32_t auraEffectType, std::int32_t basePoints,
       std::int32_t dieSides, std::uint64_t casterGuid,
       std::chrono::steady_clock::time_point expireTime, std::uint8_t visualSlot,
       std::uint32_t periodicPeriodMs, std::int32_t periodicHealthDeltaPerTick,
       std::chrono::steady_clock::time_point nextPeriodicTick)
      : _spellId(spellId), _auraEffectType(auraEffectType), _basePoints(basePoints),
        _dieSides(dieSides), _casterGuid(casterGuid), _expireTime(expireTime),
        _visualSlot(visualSlot), _periodicPeriodMs(periodicPeriodMs),
        _periodicHealthDeltaPerTick(periodicHealthDeltaPerTick),
        _nextPeriodicTick(nextPeriodicTick) {}

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

  bool IsPeriodic() const {
    return _periodicPeriodMs > 0u && _periodicHealthDeltaPerTick != 0;
  }

  bool IsPeriodicDue(std::chrono::steady_clock::time_point now) const {
    return IsPeriodic() && now >= _nextPeriodicTick;
  }

  void AdvancePeriodicTick(std::chrono::steady_clock::time_point now) {
    _nextPeriodicTick = now + std::chrono::milliseconds(_periodicPeriodMs);
  }

  std::int32_t GetMagnitude() const {
    if (_dieSides == 0)
      return _basePoints;
    return _basePoints + (_dieSides > 0 ? (_dieSides / 2) : 0);
  }

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
};

} // namespace Firelands
