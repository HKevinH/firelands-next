#include <domain/world/Creature.h>

namespace Firelands {

Creature::Creature(uint64 guid, uint32 entry, uint32 displayId)
    : WorldObject(guid), m_entry(entry), m_displayId(displayId) {}

} // namespace Firelands
