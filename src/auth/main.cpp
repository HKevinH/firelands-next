#include "AuthApplication.h"
#include <shared/Config.h>
#include <shared/Logger.h>

using namespace Firelands;

int main(int argc, char **argv) {
  Logger::Init(LoggerBuilder()
                   .WithName("firelands-auth")
                   .WithConsole(true)
                   .WithConsoleLevel(LogLevel::Info)
                   .Build());

  Config::Instance();

  if (!Config::LoadFromSearchPaths(
          "authserver.yaml", (argc > 0) ? argv[0] : nullptr,
          "FIRELANDS_AUTH_CONFIG")) {
    LOG_WARN(
        "Could not find/load authserver.yaml (cwd, exe parents, or "
        "FIRELANDS_AUTH_CONFIG); using defaults...");
  }

  return RunAuthApplication(argc, argv);
}
