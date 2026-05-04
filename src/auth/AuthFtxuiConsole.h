#pragma once

#include <functional>
#include <memory>
#include <mutex>

namespace Firelands {

class AsyncNetworkServer;
class RestAuthServer;

struct AuthFtxuiRuntime {
  std::mutex mutex;
  bool bootstrap_failed = false;
  bool services_ready = false;
  std::shared_ptr<AsyncNetworkServer> auth_server;
  std::shared_ptr<AsyncNetworkServer> realm_link_server;
  std::shared_ptr<RestAuthServer> rest_server;
};

/// Full-screen log TUI (FTXUI) for the auth server. `bootstrap_worker` runs on
/// a background thread while the UI is active so startup logs appear in the
/// log pane immediately. Exit with **Q** or **Ctrl+C** (FTXUI restores the
/// terminal).
void RunAuthFtxuiConsole(
    std::shared_ptr<AuthFtxuiRuntime> runtime,
    std::function<void(std::shared_ptr<AuthFtxuiRuntime>)> bootstrap_worker);

} // namespace Firelands
