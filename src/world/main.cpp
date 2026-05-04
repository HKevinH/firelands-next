#include <shared/Config.h>
#include <shared/Logger.h>
#include "WorldApplication.h"

using namespace Firelands;

int main(int argc, char **argv) {
  Logger::Init(LoggerBuilder()
                   .WithName("firelands-world")
                   .WithConsole(true)
                   .WithConsoleLevel(LogLevel::Info)
                   .Build());

  if (!Config::LoadFromSearchPaths(
          "worldserver.yaml", (argc > 0) ? argv[0] : nullptr,
          "FIRELANDS_WORLD_CONFIG")) {
    LOG_ERROR("Could not find/load worldserver.yaml (cwd, exe parents, or "
              "FIRELANDS_WORLD_CONFIG); exiting.");
    return 1;
  }

  return RunWorldApplication(argc, argv);
}
