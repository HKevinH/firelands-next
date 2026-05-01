#include "MapExtractorTask.h"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

namespace fs = std::filesystem;
using namespace Firelands::VMap::MapExtractor;

static void PrintUsage(const char* prog) {
    std::printf(
        "Firelands VMap Map Extractor (WoW 4.3.4.15595)\n"
        "Usage:\n"
        "  %s -d <WoW-Data-dir> -o <output-dir> [-b <build>] [-q]\n"
        "\n"
        "Options:\n"
        "  -d  Path to the WoW Data/ directory (required)\n"
        "  -o  Output directory (required). maps/ sub-dir is created automatically.\n"
        "  -b  Target build number (default: %u)\n"
        "  -q  Quiet mode — suppress per-tile progress output\n"
        "\n"
        "If no arguments are given, the interactive menu is shown.\n",
        prog, kTargetBuild);
}

int main(int argc, char* argv[]) {
    if (argc == 1) {
        // No args: show a minimal interactive prompt
        std::printf("Firelands Map Extractor (vmap pipeline)\n\n");
        std::printf("WoW Data directory: ");
        std::string dataDir; std::getline(std::cin, dataDir);
        if (dataDir.empty()) { std::printf("Cancelled.\n"); return 0; }

        std::printf("Output directory:   ");
        std::string outDir; std::getline(std::cin, outDir);
        if (outDir.empty()) { std::printf("Cancelled.\n"); return 0; }

        MapExtractorOptions opts;
        opts.dataDir   = fs::path(dataDir) / "Data";
        opts.outputDir = outDir;
        int n = RunMapExtractorTask(opts);
        return n < 0 ? 1 : 0;
    }

    MapExtractorOptions opts;
    opts.verbose = true;

    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "-d" && i + 1 < argc)
            opts.dataDir = fs::path(argv[++i]) / "Data";
        else if (arg == "-o" && i + 1 < argc)
            opts.outputDir = argv[++i];
        else if (arg == "-b" && i + 1 < argc)
            opts.build = static_cast<uint32_t>(std::atoi(argv[++i]));
        else if (arg == "-q")
            opts.verbose = false;
        else if (arg == "--help" || arg == "-h") {
            PrintUsage(argv[0]); return 0;
        } else {
            std::fprintf(stderr, "Unknown argument: %s\n", argv[i]);
            PrintUsage(argv[0]); return 1;
        }
    }

    if (opts.dataDir.empty() || opts.outputDir.empty()) {
        std::fprintf(stderr, "Error: -d and -o are required.\n");
        PrintUsage(argv[0]); return 1;
    }

    int n = RunMapExtractorTask(opts);
    return n < 0 ? 1 : 0;
}
