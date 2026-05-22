#pragma once

#include <cstdint>

namespace Firelands {
class FactionTemplateDbc;
class Player;
class Creature;
} // namespace Firelands

namespace application {

/// Returns true when `attacker` may begin a melee swing against `target` (minimal 4.3.4 rules).
bool CanMeleeAttack(Firelands::Player const &attacker, Firelands::Player const &target);

bool CanMeleeAttack(Firelands::Player const &attacker, Firelands::Creature const &target,
                    Firelands::FactionTemplateDbc const *factionTemplates);

} // namespace application
