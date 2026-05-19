#pragma once

#include <cstdint>

namespace Firelands {

/// Animation / unit emote ids (`SharedDefines.h` / client `Emotes.dbc` family).
enum Emote : uint32_t {
  EMOTE_ONESHOT_NONE = 0,
  EMOTE_ONESHOT_WAVE = 3,
  EMOTE_STATE_DANCE = 10,
  EMOTE_STATE_SLEEP = 12,
  EMOTE_STATE_SIT = 13,
  EMOTE_ONESHOT_KNEEL = 16,
  EMOTE_STATE_KNEEL = 68,
  EMOTE_STATE_READ = 433,
};

} // namespace Firelands
