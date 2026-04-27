#pragma once
#include <string>
#include <memory>

namespace Firelands {
    class WorldSession;

    class ICommandService {
    public:
        virtual ~ICommandService() = default;
        virtual bool ExecuteCommand(std::shared_ptr<WorldSession> session, const std::string& message) = 0;
        virtual bool IsCommand(const std::string& message) const = 0;
    };
}
