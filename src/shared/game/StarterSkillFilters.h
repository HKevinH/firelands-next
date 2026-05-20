#pragma once

#include <cstdint>

namespace Firelands {

/// Internal / UI meta skill lines from ref `playercreateinfo_skills` that must not
/// appear in PLAYER_SKILL update fields (client shows them as bogus professions).
bool IsMetaOrInternalStarterSkill(uint32_t skillId);

/// Riding and related skill lines granted only after training.
bool IsRidingStarterSkill(uint32_t skillId);

/// Primary and secondary profession skill lines (learned from trainers, not at create).
bool IsProfessionStarterSkill(uint32_t skillId);

/// Class spell-tab skill lines (e.g. "Mage - Fire") — not weapon/armor; clutter the skill UI.
bool IsClassSpellTabStarterSkill(uint32_t skillId);

/// Any starter skill that must not be sent on login.
bool IsExcludedStarterSkill(uint32_t skillId);

} // namespace Firelands
