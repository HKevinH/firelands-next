#pragma once

#include <shared/network/MovementInfo.h>

namespace application::combat {

struct CreatureChaseConfig {
  float runSpeedYardsPerSec = 7.0f;
  /// Stop within strike distance (~2 yd); must be less than player melee range (~8.5 yd).
  float stopDistanceYards = 2.5f;
  float zBlendPerTick = 0.35f;
};

struct CreatureChaseStepResult {
  bool moved = false;
  bool inStopRange = false;
  Firelands::MovementInfo position{};
};

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
