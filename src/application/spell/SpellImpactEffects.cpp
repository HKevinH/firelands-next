#include <application/spell/SpellImpactEffects.h>

namespace Firelands {
namespace SpellImpactEffects {

uint32 ResolveImpactKitForSpell(SpellDefinition const &def,
                                SpellVisualDbc const &spellVisualDbc) {
  uint32 const visuals[2] = {def.spellVisualId1, def.spellVisualId0};
  for (uint32 visualId : visuals) {
    if (visualId == 0u)
      continue;
    if (uint32 const kit = spellVisualDbc.ResolveImpactKitId(visualId); kit != 0u)
      return kit;
  }
  return 0u;
}

} // namespace SpellImpactEffects
} // namespace Firelands
