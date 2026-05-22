#pragma once

#include <shared/network/WorldPacket.h>

namespace Firelands {
namespace PlaySpellVisualKitWire {

/// Builds `SMSG_PLAY_SPELL_VISUAL_KIT` (4.3.4 / 15595). Plays `kitRecId` on `unitGuid`.
/// `kitType` and `durationMs` match reference `Unit::SendPlaySpellVisualKit` defaults (0).
void BuildPlaySpellVisualKit(WorldPacket &out, uint64 unitGuid, int32 kitRecId,
                             int32 kitType = 0, uint32 durationMs = 0);

} // namespace PlaySpellVisualKitWire
} // namespace Firelands
