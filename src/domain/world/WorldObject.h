#pragma once

#include <shared/Common.h>
#include <shared/network/MovementInfo.h>

namespace Firelands {

class WorldObject {
public:
  explicit WorldObject(uint64 guid) : m_guid(guid) {}
  virtual ~WorldObject() = default;

  uint64 GetGuid() const { return m_guid; }

  void SetPosition(const MovementInfo &pos) { m_position = pos; }
  const MovementInfo &GetPosition() const { return m_position; }

  float GetX() const { return m_position.x; }
  float GetY() const { return m_position.y; }
  float GetZ() const { return m_position.z; }
  float GetOrientation() const { return m_position.orientation; }

protected:
  uint64 m_guid;
  MovementInfo m_position;
};

} // namespace Firelands
