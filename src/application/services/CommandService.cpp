#include "CommandService.h"
#include <infrastructure/network/sessions/WorldSession.h>
#include <sstream>
#include <iterator>

namespace Firelands {

    CommandService::CommandService() {
        RegisterCommand("gps", [this](auto s, auto a) { return HandleGps(s, a); });
        RegisterCommand("tele", [this](auto s, auto a) { return HandleTele(s, a); });
        RegisterCommand("help", [this](auto s, auto a) { return HandleHelp(s, a); });
    }

    void CommandService::RegisterCommand(const std::string& name, CommandHandler handler) {
        _commands[name] = std::move(handler);
    }

    bool CommandService::IsCommand(const std::string& message) const {
        return !message.empty() && (message[0] == '.' || message[0] == '!');
    }

    bool CommandService::ExecuteCommand(std::shared_ptr<WorldSession> session, const std::string& message) {
        if (!IsCommand(message)) return false;

        std::string cmdStr = message.substr(1);
        std::istringstream iss(cmdStr);
        std::vector<std::string> args{std::istream_iterator<std::string>{iss},
                                     std::istream_iterator<std::string>{}};

        if (args.empty()) return false;

        std::string cmdName = args[0];
        args.erase(args.begin());

        auto it = _commands.find(cmdName);
        if (it != _commands.end()) {
            return it->second(session, args);
        }

        session->SendNotification("Unknown command: " + cmdName);
        return false;
    }

    bool CommandService::HandleGps(std::shared_ptr<WorldSession> session, const std::vector<std::string>&) {
        const auto& pos = session->GetPosition();
        std::string msg = "Current Position: X=" + std::to_string(pos.x) + 
                          " Y=" + std::to_string(pos.y) + 
                          " Z=" + std::to_string(pos.z) + 
                          " O=" + std::to_string(pos.orientation);
        session->SendNotification(msg);
        return true;
    }

    bool CommandService::HandleTele(std::shared_ptr<WorldSession> session, const std::vector<std::string>& args) {
        try {
            if (args.size() < 3) {
                session->SendNotification("Usage: .tele <x> <y> <z> [mapId]");
                return false;
            }

            float x = std::stof(args[0]);
            float y = std::stof(args[1]);
            float z = std::stof(args[2]);
            uint32 mapId = (args.size() > 3) ? static_cast<uint32>(std::stoul(args[3])) : 0;

            session->TeleportTo(mapId, x, y, z);
            return true;
        } catch (const std::exception& e) {
            session->SendNotification("Error: Invalid arguments.");
            return false;
        }
    }

    bool CommandService::HandleHelp(std::shared_ptr<WorldSession> session, const std::vector<std::string>&) {
        session->SendNotification("Available commands: .gps, .tele, .help");
        return true;
    }

}
