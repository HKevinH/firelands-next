#include <application/spell/SpellHitEffects.h>
#include <application/spell/SpellManager.h>

namespace Firelands {
namespace SpellHitEffects {

uint64 ResolvePrimarySpellHitUnitGuid(uint32 clientTargetFlags, uint64 casterGuid,
                                      uint64 unitTargetGuid) {
  if ((clientTargetFlags & SpellCastWire::ClientTargetPrimaryGuidMask) != 0 &&
      unitTargetGuid != 0u)
    return unitTargetGuid;
  return casterGuid;
}

void ApplyImmediateHealthFromDefinition(SpellDefinition const *def, uint64 hitGuid,
                                        SpellCastOutcome *out) {
  if (!out || !def || def->immediateHealthEffectDelta == 0)
    return;
  out->hasDirectHealthEffect = true;
  out->directHealthTargetGuid = hitGuid;
  out->directHealthDelta = def->immediateHealthEffectDelta;
}

} // namespace SpellHitEffects
} // namespace Firelands
