#pragma once

#include <functional>
#include <memory>
#include <mutex>

namespace Firelands {

class AsyncNetworkServer;
class WorldInteractiveConsole;
class CommandService;

/// Shared state between the FTXUI main thread (render + tick) and the
/// background bootstrap thread that builds network services.
struct WorldFtxuiRuntime {
  std::mutex mutex;
  bool bootstrap_failed = false;
  bool services_ready = false;
  std::shared_ptr<AsyncNetworkServer> world_server;
  std::shared_ptr<WorldInteractiveConsole> interactive_console;
  std::shared_ptr<CommandService> command_service;
};

/// Full-screen terminal UI (FTXUI): scrollable log pane and a fixed bottom
/// command strip. `bootstrap_worker` runs on a background thread while the UI
/// is already active so startup logs appear in the log pane immediately.
/// Replaces the stdout color sink for the duration of the run.
void RunWorldFtxuiConsole(
    std::shared_ptr<WorldFtxuiRuntime> runtime,
    std::function<void(std::shared_ptr<WorldFtxuiRuntime>)> bootstrap_worker);

} // namespace Firelands
