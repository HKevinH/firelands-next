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

} // namespace application::combat
