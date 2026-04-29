#pragma once

#include <domain/world/WorldObject.h>
#include <shared/Common.h>

namespace Firelands {

/// In-world unit (NPC / pet / summon). Gameplay AI is driven from Lua hooks, not
/// Smart-Script-style DB rows.
class Creature : public WorldObject {
public:
  Creature(uint64 guid, uint32 entry, uint32 displayId);

  uint32 GetEntry() const { return m_entry; }
  uint32 GetDisplayId() const { return m_displayId; }

private:
  uint32 m_entry;
  uint32 m_displayId;
};

} // namespace Firelands
