#pragma once
#include <application/ports/ICommandService.h>
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace Firelands {

class CommandService : public ICommandService {
public:
  CommandService();
  bool ExecuteCommand(std::shared_ptr<WorldSession> session,
                      const std::string &message) override;
  bool IsCommand(const std::string &message) const override;

private:
  using CommandHandler = std::function<bool(std::shared_ptr<WorldSession>,
                                            const std::vector<std::string> &)>;

  void RegisterCommand(const std::string &name, CommandHandler handler);

  // Handlers
  bool HandleGps(std::shared_ptr<WorldSession> session,
                 const std::vector<std::string> &args);
  bool HandleTele(std::shared_ptr<WorldSession> session,
                  const std::vector<std::string> &args);
  bool HandleHelp(std::shared_ptr<WorldSession> session,
                  const std::vector<std::string> &args);

  std::map<std::string, CommandHandler> _commands;
};

} // namespace Firelands
