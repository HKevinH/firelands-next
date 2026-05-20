#include <shared/network/AuraUpdateWire.h>

namespace Firelands {
namespace AuraUpdateWire {

namespace {

uint8 ResolveActiveEffectMask(AuraApplyParams const &params) {
  if (params.activeEffectMask != 0u)
    return params.activeEffectMask;
  if (params.effectIndex < 3u)
    return static_cast<uint8>(1u << params.effectIndex);
  return kEffectIndex0;
}

void AppendEffectAmounts(WorldPacket &out, uint16 flags, AuraApplyParams const &params) {
  if ((flags & kAnyEffectAmountSent) == 0u || !params.sendEffectAmount)
    return;
  for (uint8 i = 0; i < 3u; ++i) {
    if ((flags & static_cast<uint16>(1u << i)) == 0u)
      continue;
    int32 const amount =
        (i == params.effectIndex) ? params.effectAmount : 0;
    out.Append<int32>(amount);
  }
}

void AppendAuraEntry(WorldPacket &out, uint64 unitGuid, AuraApplyParams const &params) {
  uint8 const effectMask = ResolveActiveEffectMask(params);
  bool const selfCast = params.casterGuid != 0u && params.casterGuid == unitGuid;
  uint16 flags = 0;
  if ((effectMask & 0x07u) != 0u)
    flags |= static_cast<uint16>(effectMask & 0x07u);
  if (selfCast)
    flags |= kCaster;
  if (params.durationMs > 0u)
    flags |= kDuration;
  if (params.isNegative)
    flags |= kNegative;
  else
    flags |= kPositive;
  if (params.sendEffectAmount)
    flags |= kAnyEffectAmountSent;

  out.Append<uint8>(params.visualSlot);
  out.Append<int32>(static_cast<int32>(params.spellId));
  if (params.spellId == 0u)
    return;

  out.Append<uint16>(flags);
  out.Append<uint8>(params.casterLevel);
  out.Append<uint8>(params.stackCount > 0 ? params.stackCount : 1);

  if (!selfCast && params.casterGuid != 0u)
    out.AppendPackGUID(params.casterGuid);

  if ((flags & kDuration) != 0u) {
    int32 const maxDuration = static_cast<int32>(params.durationMs);
    int32 const remaining = static_cast<int32>(
        params.explicitRemaining ? params.remainingMs
                                 : (params.remainingMs > 0u ? params.remainingMs
                                                            : params.durationMs));
    out.Append<int32>(maxDuration);
    out.Append<int32>(remaining);
  }

  AppendEffectAmounts(out, flags, params);
}

} // namespace

void BuildAuraApply(WorldPacket &out, uint64 unitGuid, AuraApplyParams const &params) {
  out = WorldPacket(SMSG_AURA_UPDATE, 64);
  out.AppendPackGUID(unitGuid);
  AppendAuraEntry(out, unitGuid, params);
}

void BuildAuraUpdateAll(WorldPacket &out, uint64 unitGuid,
                        std::vector<AuraApplyParams> const &auras) {
  size_t const reserve = 32 + auras.size() * 48;
  out = WorldPacket(SMSG_AURA_UPDATE_ALL, reserve);
  out.AppendPackGUID(unitGuid);
  for (AuraApplyParams const &params : auras)
    AppendAuraEntry(out, unitGuid, params);
}

void BuildAuraExpire(WorldPacket &out, uint64 unitGuid, AuraApplyParams const &params) {
  AuraApplyParams expire = params;
  expire.remainingMs = 0u;
  expire.explicitRemaining = true;
  BuildAuraApply(out, unitGuid, expire);
}

void BuildAuraRemove(WorldPacket &out, uint64 unitGuid, uint8 visualSlot) {
  out = WorldPacket(SMSG_AURA_UPDATE, 24);
  out.AppendPackGUID(unitGuid);
  AuraApplyParams removal{};
  removal.visualSlot = visualSlot;
  removal.spellId = 0u;
  AppendAuraEntry(out, unitGuid, removal);
}

} // namespace AuraUpdateWire
} // namespace Firelands
