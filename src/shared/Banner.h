#ifndef FIRELANDS_SHARED_BANNER_H
#define FIRELANDS_SHARED_BANNER_H

#include <iostream>
#include <string>

namespace Firelands {

    enum class BannerType {
        Auth,
        World,
        Tools
    };

    /**
     * @brief Prints a cool ASCII art banner for the Firelands project.
     * Uses ANSI escape codes for coloring if supported.
     */
    inline void PrintBanner(BannerType type = BannerType::Auth) {
        // ANSI Colors
        const std::string RED      = "\033[31m";
        const std::string ORANGE   = "\033[38;5;208m";
        const std::string YELLOW   = "\033[33m";
        const std::string CYAN     = "\033[36m";
        const std::string GREEN    = "\033[32m";
        const std::string MAGENTA  = "\033[35m";
        const std::string BLUE     = "\033[34m";
        const std::string WHITE    = "\033[37m";
        const std::string RESET    = "\033[0m";
        const std::string BOLD     = "\033[1m";

        std::string mainColor = ORANGE;
        std::string label = "Project Core";

        switch (type) {
            case BannerType::Auth:
                mainColor = MAGENTA;
                label = " AUTH SERVER ";
                break;
            case BannerType::World:
                mainColor = GREEN;
                label = " WORLD SERVER ";
                break;
            case BannerType::Tools:
                mainColor = CYAN;
                label = " DEVELOPER TOOLS ";
                break;
        }

        std::cout << mainColor << BOLD << R"(
    ███████╗██╗██████╗ ███████╗██╗      █████╗ ███╗   ██╗██████╗ ███████╗
    ██╔════╝██║██╔══██╗██╔════╝██║     ██╔══██╗████╗  ██║██╔══██╗██╔════╝
    █████╗  ██║██████╔╝█████╗  ██║     ███████║██╔██╗ ██║██║  ██║███████╗
    ██╔══╝  ██║██╔══██╗██╔══╝  ██║     ██╔══██║██║╚██╗██║██║  ██║╚════██║
    ██║     ██║██║  ██║███████╗███████╗██║  ██║██║ ╚████║██████╔╝███████║
    ╚═╝     ╚═╝╚═╝  ╚═╝╚══════╝╚══════╝╚═╝  ╚═╝╚═╝  ╚═══╝╚═════╝ ╚══════╝
        )" << YELLOW << "           Cataclysm WoW Emulator | " << BOLD << mainColor << label << YELLOW << " | Build 15595" << RESET << std::endl;
    }

} // namespace Firelands

#endif // FIRELANDS_SHARED_BANNER_H
