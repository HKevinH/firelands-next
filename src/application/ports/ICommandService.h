#pragma once
#include <shared/game/AccessLevel.h>
#include <memory>
#include <string>

namespace Firelands {
class WorldSession;

class ICommandService {
public:
  virtual ~ICommandService() = default;
  virtual bool ExecuteCommand(std::shared_ptr<WorldSession> session,
                              const std::string &message,
                              PrivilegeOrigin origin = PrivilegeOrigin::GameClient) = 0;
  virtual bool IsCommand(const std::string &message) const = 0;
};
} // namespace Firelands
