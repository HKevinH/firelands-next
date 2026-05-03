#pragma once

#include <domain/world/WorldObject.h>
#include <shared/Common.h>

namespace Firelands {

/// In-world unit (NPC / pet / summon). Gameplay AI is driven from Lua hooks, not
/// Smart-Script-style DB rows.
class Creature : public WorldObject {
public:
  /// `maxHealth` seeds runtime HP for spell/effect MVP (template HP later).
  Creature(uint64 guid, uint32 entry, uint32 displayId, uint32 maxHealth = 100u);

  uint32 GetEntry() const { return m_entry; }
  uint32 GetDisplayId() const { return m_displayId; }

  uint32 GetLiveHealth() const { return m_liveHealth; }
  uint32 GetLiveMaxHealth() const { return m_liveMaxHealth; }
  void ApplyHealthDelta(int32 delta);

private:
  uint32 m_entry;
  uint32 m_displayId;
  uint32 m_liveHealth = 1;
  uint32 m_liveMaxHealth = 1;
};

} // namespace Firelands
