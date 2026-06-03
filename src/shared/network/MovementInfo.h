#pragma once

#include <shared/Common.h>

namespace Firelands {

/// Client movement state (`MovementFlags` / `MovementFlags2` in MovementFlags.h).
struct MovementInfo {
  uint32 flags = 0;
  uint16 flags2 = 0;
  uint32 time = 0;
  float x = 0.0f, y = 0.0f, z = 0.0f, orientation = 0.0f;
  uint32 fallTime = 0;

  // Jump / fall arc (present when the client sends MSEHasFallData; the extra
  // direction fields only when MSEHasFallDirection). Needed to relay jumps so
  // observers render the arc instead of a slide.
  bool hasFallData = false;
  bool hasFallDirection = false;
  float jumpVerticalSpeed = 0.0f;   // MSEFallVerticalSpeed (z speed)
  float jumpSinAngle = 0.0f;        // MSEFallSinAngle
  float jumpCosAngle = 0.0f;        // MSEFallCosAngle
  float jumpHorizontalSpeed = 0.0f; // MSEFallHorizontalSpeed (xy speed)
};

} // namespace Firelands
