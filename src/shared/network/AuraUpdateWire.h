#pragma once

#include <shared/Common.h>
#include <shared/network/WorldPacket.h>

namespace Firelands {
namespace AuraUpdateWire {

/// Cataclysm 4.3.4 `AuraFlag` (WowPacketParser / cmangos). On the wire these are `uint16`
/// since build 4.2.0.14333, not a single byte.
enum AuraFlag : uint16 {
  kEmpty = 0x0000,
  kEffectIndex0 = 0x0001,
  kEffectIndex1 = 0x0002,
  kEffectIndex2 = 0x0004,
  kNotCaster = 0x0008,
  kPositive = 0x0010,
  kDuration = 0x0020,
  kScalable = 0x0040,
  kNegative = 0x0080,
};

struct AuraApplyParams {
  uint8 visualSlot = 0;
  uint32 spellId = 0;
  /// `SpellEffect.dbc` effect index (0–2) for the apply-aura row.
  uint8 effectIndex = 0;
  uint8 casterLevel = 1;
  uint8 stackCount = 1;
  uint64 casterGuid = 0;
  uint32 durationMs = 0;
  uint32 remainingMs = 0;
  bool isNegative = false;
};

/// Builds `SMSG_AURA_UPDATE` (apply or refresh) for `unitGuid`.
void BuildAuraApply(WorldPacket &out, uint64 unitGuid, AuraApplyParams const &params);

/// Builds `SMSG_AURA_UPDATE` telling the client to remove the aura in `visualSlot`.
void BuildAuraRemove(WorldPacket &out, uint64 unitGuid, uint8 visualSlot);

} // namespace AuraUpdateWire
} // namespace Firelands
