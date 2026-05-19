#pragma once

#include <chrono>

namespace Firelands {

/// Expires auras and runs periodic ticks on all maps; broadcasts aura/health updates.
void TickMapAuras(std::chrono::steady_clock::time_point now);

} // namespace Firelands
