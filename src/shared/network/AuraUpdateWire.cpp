#include <shared/network/AuraUpdateWire.h>

namespace Firelands {
namespace AuraUpdateWire {

void BuildAuraApply(WorldPacket &out, uint64 unitGuid, AuraApplyParams const &params) {
  out = WorldPacket(SMSG_AURA_UPDATE, 64);
  out.AppendPackGUID(unitGuid);

  bool const selfCast = params.casterGuid != 0u && params.casterGuid == unitGuid;
  uint16 flags = 0;
  if (params.effectIndex < 3u)
    flags |= static_cast<uint16>(1u << params.effectIndex);
  if (selfCast)
    flags |= kNotCaster;
  if (params.durationMs > 0u)
    flags |= kDuration;
  if (params.isNegative)
    flags |= kNegative;
  else
    flags |= kPositive;

  out.Append<uint8>(params.visualSlot);
  out.Append<int32>(static_cast<int32>(params.spellId));
  out.Append<uint16>(flags);
  out.Append<uint8>(params.casterLevel);
  out.Append<uint8>(params.stackCount > 0 ? params.stackCount : 1);

  // Caster GUID only when `AuraFlag::NotCaster` is clear (self-cast omits it).
  if (!selfCast && params.casterGuid != 0u)
    out.AppendPackGUID(params.casterGuid);

  if ((flags & kDuration) != 0u) {
    int32 const maxDuration = static_cast<int32>(params.durationMs);
    int32 const remaining = static_cast<int32>(
        params.remainingMs > 0u ? params.remainingMs : params.durationMs);
    out.Append<int32>(maxDuration);
    out.Append<int32>(remaining);
  }
}

void BuildAuraRemove(WorldPacket &out, uint64 unitGuid, uint8 visualSlot) {
  out = WorldPacket(SMSG_AURA_UPDATE, 24);
  out.AppendPackGUID(unitGuid);
  out.Append<uint8>(visualSlot);
  out.Append<int32>(0);
}

} // namespace AuraUpdateWire
} // namespace Firelands
