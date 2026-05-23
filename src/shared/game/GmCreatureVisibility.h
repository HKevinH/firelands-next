#pragma once

#include <shared/game/CreatureExtraFlags.h>
#include <shared/game/PhaseShift.h>
#include <shared/game/UnitFieldFlags.h>

#include <cstdint>

namespace Firelands {

/// Default invisible model for `CREATURE_FLAG_EXTRA_TRIGGER` spawns (TrinityCore fallback).
inline constexpr uint32_t kInvisibleTriggerDisplayId = 11686u;

/// When GM tag is on, the viewer sees every creature regardless of phase shift.
inline bool CreatureVisibleToViewer(PhaseShift const &viewerPhase,
                                    PhaseShift const &creaturePhase,
                                    bool gmSeeAllCreatures) {
  if (gmSeeAllCreatures)
    return true;
  return viewerPhase.CanSee(creaturePhase);
}

/// Strips `UNIT_FIELD_FLAG_NOT_SELECTABLE` so GMs can click script triggers and similar units.
inline uint32_t WireUnitFieldFlagsForCreature(uint32_t templateUnitFieldFlags,
                                              bool gmSeeAllCreatures) {
  if (!gmSeeAllCreatures)
    return templateUnitFieldFlags;
  return templateUnitFieldFlags & ~kUnitFieldFlagNotSelectable;
}

/// Uses template `npcflag` as sent to the client (skips quest-giver wire adjustments).
inline uint32_t WireNpcFlagsForCreature(uint32_t templateNpcFlags,
                                        uint32_t questAdjustedNpcFlags,
                                        bool gmSeeAllCreatures) {
  return gmSeeAllCreatures ? templateNpcFlags : questAdjustedNpcFlags;
}

/// `ObjectMgr::ChooseDisplayId` + `Unit.cpp` trigger wire: invisible for players, visible for GMs.
inline uint32_t WireDisplayIdForCreature(uint32_t storedDisplayId, uint32_t extraFlags,
                                         bool gmSeeAllCreatures) {
  if ((extraFlags & kCreatureExtraFlagTrigger) == 0u)
    return storedDisplayId;
  return gmSeeAllCreatures ? storedDisplayId : kInvisibleTriggerDisplayId;
}

} // namespace Firelands
