#include <shared/network/SpellPeriodicAuraLogWire.h>
#include <shared/game/SpellAuraTypes.h>

namespace Firelands {
namespace SpellPeriodicAuraLogWire {

void BuildPeriodicHeal(WorldPacket &out, uint64 targetGuid, uint64 casterGuid,
                       uint32 spellId, uint32 healAmount, uint32 overHeal,
                       uint32 absorb, bool critical) {
  out = WorldPacket(SMSG_PERIODICAURALOG, 48);
  out.AppendPackGUID(targetGuid);
  out.AppendPackGUID(casterGuid);
  out.Append<uint32>(spellId);
  out.Append<uint32>(1u);
  out.Append<uint32>(kSpellAuraPeriodicHeal);
  out.Append<uint32>(healAmount);
  out.Append<uint32>(overHeal);
  out.Append<uint32>(absorb);
  out.Append<uint8>(critical ? 1u : 0u);
}

} // namespace SpellPeriodicAuraLogWire
} // namespace Firelands
