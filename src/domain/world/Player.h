#pragma once

#include <application/ports/IMapNotifier.h>
#include <domain/world/WorldObject.h>
#include <memory>

namespace Firelands {

class Player : public WorldObject {
public:
  explicit Player(uint64 guid, std::shared_ptr<IMapNotifier> notifier)
      : WorldObject(guid), m_notifier(std::move(notifier)) {}

  std::shared_ptr<IMapNotifier> GetNotifier() const { return m_notifier; }

private:
  std::shared_ptr<IMapNotifier> m_notifier;
};

} // namespace Firelands
