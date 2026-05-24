#pragma once

#include <shared/network/MovementInfo.h>

namespace application::combat {

struct CreatureChaseConfig {
  float runSpeedYardsPerSec = 7.0f;
  /// Halt this far from the target center (Cataclysm chase contact distance).
  float stopDistanceYards = 1.0f;
  float zBlendPerTick = 0.35f;
};

struct CreatureChaseStepResult {
  bool moved = false;
  bool inStopRange = false;
  Firelands::MovementInfo position{};
};

/// Last point on the approach line, `stopDistanceYards` from the target center.
Firelands::MovementInfo ComputeChaseStandPosition(Firelands::MovementInfo const &from,
                                       float targetX, float targetY, float targetZ,
                                       float stopDistanceYards);

/// Advances `current` toward (`targetX`,`targetY`,`targetZ`) by `deltaSeconds`.
CreatureChaseStepResult StepCreatureTowardTarget(Firelands::MovementInfo const &current,
                                                float targetX, float targetY,
                                                float targetZ, float deltaSeconds,
                                                CreatureChaseConfig const &config);

/// Simulates repeated `StepCreatureTowardTarget` for up to `maxDeltaSeconds` (one tick per
/// 0.2s slice). Used to build a single client spline instead of restarting animation every tick.
CreatureChaseStepResult ProjectCreatureTowardTarget(Firelands::MovementInfo const &current,
                                                  float targetX, float targetY,
                                                  float targetZ, float maxDeltaSeconds,
                                                  CreatureChaseConfig const &config);

/// Ref `ChaseMovementGenerator`: replan when `target->GetPosition() != _lastTargetPosition`.
bool ChaseTargetRelocated(float lastX, float lastY, float lastZ, float newX, float newY,
                          float newZ, float thresholdYards = 0.5f);

} // namespace application::combat
