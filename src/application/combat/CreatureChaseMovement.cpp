#include "CreatureChaseMovement.h"

#include <shared/network/MovementFlags.h>
#include <cmath>

using Firelands::MOVEMENTFLAG_FORWARD;
using Firelands::MOVEMENTFLAG_NONE;
using Firelands::MovementInfo;

namespace application::combat {

namespace {

float DistanceSquared2d(float ax, float ay, float bx, float by) {
  float const dx = ax - bx;
  float const dy = ay - by;
  return dx * dx + dy * dy;
}

float OrientationToward(float fromX, float fromY, float toX, float toY) {
  return std::atan2(toY - fromY, toX - fromX);
}

} // namespace

MovementInfo ComputeChaseStandPosition(MovementInfo const &from, float targetX,
                                       float targetY, float targetZ,
                                       float stopDistanceYards) {
  float const dx = from.x - targetX;
  float const dy = from.y - targetY;
  float const distSq = dx * dx + dy * dy;

  MovementInfo pos = from;
  if (distSq < 1e-8f) {
    pos.orientation = OrientationToward(from.x, from.y, targetX, targetY);
    pos.flags = MOVEMENTFLAG_NONE;
    return pos;
  }

  float const dist = std::sqrt(distSq);
  float const standDist = std::min(dist, stopDistanceYards);
  float const nx = dx / dist;
  float const ny = dy / dist;

  pos.x = targetX + nx * standDist;
  pos.y = targetY + ny * standDist;
  pos.z = from.z + (targetZ - from.z) * (standDist / dist);
  pos.orientation = OrientationToward(pos.x, pos.y, targetX, targetY);
  pos.flags = MOVEMENTFLAG_NONE;
  return pos;
}

CreatureChaseStepResult StepCreatureTowardTarget(Firelands::MovementInfo const &current,
                                                float targetX, float targetY,
                                                float targetZ,
                                                float deltaSeconds,
                                                CreatureChaseConfig const &config) {
  CreatureChaseStepResult result{};
  result.position = current;

  float const stopSq = config.stopDistanceYards * config.stopDistanceYards;
  float const distSq =
      DistanceSquared2d(current.x, current.y, targetX, targetY);
  if (distSq <= stopSq) {
    result.inStopRange = true;
    result.position.orientation =
        OrientationToward(current.x, current.y, targetX, targetY);
    result.position.flags = MOVEMENTFLAG_NONE;
    result.moved = false;
    return result;
  }

  float const dist = std::sqrt(distSq);
  float const maxStep = std::max(0.01f, config.runSpeedYardsPerSec * deltaSeconds);
  float const allowed = std::max(0.f, dist - config.stopDistanceYards);
  float const step = std::min(maxStep, allowed);

  float const nx = current.x + (targetX - current.x) / dist * step;
  float const ny = current.y + (targetY - current.y) / dist * step;
  float const dz = targetZ - current.z;
  float const nz =
      std::abs(dz) > 0.01f ? current.z + dz * config.zBlendPerTick : current.z;

  result.position.x = nx;
  result.position.y = ny;
  result.position.z = nz;
  result.position.orientation = OrientationToward(nx, ny, targetX, targetY);
  result.position.flags = MOVEMENTFLAG_FORWARD;
  result.moved = step > 0.01f;
  result.inStopRange = false;
  return result;
}

CreatureChaseStepResult ProjectCreatureTowardTarget(MovementInfo const &current,
                                                  float targetX, float targetY,
                                                  float targetZ, float maxDeltaSeconds,
                                                  CreatureChaseConfig const &config) {
  constexpr float kSimTickSeconds = 0.2f;

  CreatureChaseStepResult result{};
  result.position = current;
  result.moved = false;
  result.inStopRange = false;

  float remaining = std::max(0.f, maxDeltaSeconds);
  while (remaining > 1e-4f) {
    float const slice = std::min(remaining, kSimTickSeconds);
    auto const step =
        StepCreatureTowardTarget(result.position, targetX, targetY, targetZ, slice, config);
    result = step;
    if (step.inStopRange || !step.moved)
      break;
    remaining -= slice;
  }
  return result;
}

bool ChaseTargetRelocated(float lastX, float lastY, float lastZ, float newX, float newY,
                          float newZ, float thresholdYards) {
  float const dx = newX - lastX;
  float const dy = newY - lastY;
  float const dz = newZ - lastZ;
  float const thresholdSq = thresholdYards * thresholdYards;
  return (dx * dx + dy * dy + dz * dz) > thresholdSq;
}

} // namespace application::combat
