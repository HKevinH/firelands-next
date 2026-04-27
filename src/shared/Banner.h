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
     * 
     * @param type The type of banner to print.
     * @param fixed If true, clears the screen and sets a scrolling region below the banner.
     */
    inline void PrintBanner(BannerType type = BannerType::Auth, bool fixed = false) {
        // ANSI Colors - Intense Orange Palette
        const std::string ORANGE_VIBRANT = "\033[38;5;202m";
        const std::string ORANGE_BRIGHT  = "\033[38;5;208m";
        const std::string ORANGE_WARM    = "\033[38;5;214m";
        const std::string BLUE_HIGHLIGHT = "\033[38;5;39m";  // Complementary Deep Sky Blue
        const std::string WHITE          = "\033[37m";
        const std::string RESET          = "\033[0m";
        const std::string BOLD           = "\033[1m";
        const std::string CLEAR_SCREEN   = "\033[2J";
        const std::string CURSOR_HOME    = "\033[H";

        if (fixed) {
            std::cout << CLEAR_SCREEN << CURSOR_HOME;
        }

        std::string mainColor = ORANGE_BRIGHT;
        std::string label = " PROJECT INTERNALS ";

        switch (type) {
            case BannerType::Auth:
                mainColor = ORANGE_VIBRANT;
                label = " AUTH SERVER ";
                break;
            case BannerType::World:
                mainColor = ORANGE_BRIGHT;
                label = " WORLD SERVER ";
                break;
            case BannerType::Tools:
                mainColor = ORANGE_WARM;
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
        )" << WHITE << "           Cataclysm WoW Emulator | " << BOLD << BLUE_HIGHLIGHT << label << RESET << WHITE << " | Build 15595" << RESET << std::endl;

        if (fixed) {
            // Set scrolling region: from line 10 to the end of the screen
            // and move cursor to line 10 to start logging.
            std::cout << "\033[10;r" << "\033[10;1H" << std::flush;
        }
    }

} // namespace Firelands

#endif // FIRELANDS_SHARED_BANNER_H
