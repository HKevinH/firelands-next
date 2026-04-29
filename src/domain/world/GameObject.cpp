#include <domain/world/GameObject.h>

namespace Firelands {

GameObject::GameObject(uint64 guid, uint32 entry)
    : WorldObject(guid), m_entry(entry) {}

} // namespace Firelands
