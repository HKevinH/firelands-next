#include <shared/game/ShapeshiftForms.h>

namespace Firelands {

bool IsWarriorStanceSpell(uint32 spellId) {
  switch (spellId) {
  case kSpellBattleStance:
  case kSpellDefensiveStance:
  case kSpellBerserkerStance:
    return true;
  default:
    return false;
  }
}

uint8 WarriorStanceFormForSpell(uint32 spellId) {
  switch (spellId) {
  case kSpellBattleStance:
    return FORM_BATTLESTANCE;
  case kSpellDefensiveStance:
    return FORM_DEFENSIVESTANCE;
  case kSpellBerserkerStance:
    return FORM_BERSERKERSTANCE;
  default:
    return FORM_NONE;
  }
}

uint32 StanceSpellForForm(uint8 form) {
  switch (form) {
  case FORM_BATTLESTANCE:
    return kSpellBattleStance;
  case FORM_DEFENSIVESTANCE:
    return kSpellDefensiveStance;
  case FORM_BERSERKERSTANCE:
    return kSpellBerserkerStance;
  default:
    return 0u;
  }
}

bool TryGetWarriorAbilityStanceRequirement(uint32 spellId, uint32 &stances,
                                           uint32 &stancesNot) {
  // Representative Cataclysm 4.3.4 warrior abilities locked to a stance. This is the tuning
  // point for stance gating; extend as more abilities are implemented.
  switch (spellId) {
  case 100u:  // Charge — Battle Stance only (no Juggernaut/Warbringer talents)
  case 7384u: // Overpower — Battle Stance only
    stances = StanceMaskFromForm(FORM_BATTLESTANCE);
    stancesNot = 0u;
    return true;
  case 20252u: // Intercept — Berserker Stance only
    stances = StanceMaskFromForm(FORM_BERSERKERSTANCE);
    stancesNot = 0u;
    return true;
  case 871u: // Shield Wall — not usable in Berserker Stance
    stances = 0u;
    stancesNot = StanceMaskFromForm(FORM_BERSERKERSTANCE);
    return true;
  default:
    return false;
  }
}

uint32 RageRetainedOnStanceSwitch(uint32 currentRage) {
  (void)currentRage;
  // Baseline: dump all rage on stance change.
  // TODO(Preservation): Tactical Mastery keeps min(currentRage, talentCap).
  return 0u;
}

bool GetWarriorStanceDamageMods(uint8 form, int32 &damageDonePctPoints,
                                int32 &damageTakenPctPoints) {
  switch (form) {
  case FORM_DEFENSIVESTANCE:
    // Tank stance: takes and deals less damage.
    damageDonePctPoints = -10;
    damageTakenPctPoints = -10;
    return true;
  case FORM_BERSERKERSTANCE:
    // Offensive stance: deals more but takes more.
    damageDonePctPoints = 10;
    damageTakenPctPoints = 10;
    return true;
  case FORM_BATTLESTANCE:
  default:
    damageDonePctPoints = 0;
    damageTakenPctPoints = 0;
    return false;
  }
}

} // namespace Firelands
