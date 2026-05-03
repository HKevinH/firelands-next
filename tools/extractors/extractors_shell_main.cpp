#include "ExtractorsFtxui.h"

#include <iostream>
#include <string>

int main(int argc, char **argv) {
  if (argc >= 2) {
    const std::string a(argv[1]);
    if (a == "-h" || a == "--help") {
      std::cout
          << "firelands-extractors — fullscreen TUI to configure and run client-data tasks.\n"
             "\n"
             "Run with no arguments to open the interactive launcher (banner + console).\n"
             "\n"
             "Non-interactive / CI (no TTY):\n"
             "  firelands-dbc-extractor   --data <WoW/Data> --out <dir> [--list-mpqs]\n"
             "  firelands-map-extractor   --data <WoW/Data> --out <dir> [--list-mpqs]\n";
      return 0;
    }
    std::cerr << "Unknown argument. Use --help.\n";
    return 2;
  }
  return firelands::extract::RunExtractorsFtxui();
}
