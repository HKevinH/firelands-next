#pragma once

#include <domain/world/WorldObject.h>
#include <shared/Common.h>

namespace Firelands {

/// Interactive world object (chest, door, node). Script hooks mirror ref
/// `GameObject` + `GameObjectAI` via Lua instead of C++ scripts only.
class GameObject : public WorldObject {
public:
  GameObject(uint64 guid, uint32 entry);

  uint32 GetEntry() const { return m_entry; }

private:
  uint32 m_entry;
};

} // namespace Firelands
