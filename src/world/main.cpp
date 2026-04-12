#include <shared/Banner.h>
#include <shared/Logger.h>
#include <infrastructure/network/asio/AsyncNetworkServer.h>
#include <infrastructure/network/sessions/WorldSession.h>
#include <infrastructure/persistence/MySqlAccountRepository.h>
#include <infrastructure/persistence/MySqlCharacterRepository.h>
#include <application/services/CharacterService.h>
#include <conncpp.hpp>
#include <thread>
#include <chrono>

using namespace Firelands;

int main() {
    Logger::Init(
        LoggerBuilder()
            .WithName("firelands-world")
            .WithConsole(true)
            .WithConsoleLevel(LogLevel::Info)
            .WithFile(true, "logs/firelands-world.log")
            .WithFileLevel(LogLevel::Debug)
            .WithRotatingFile(10 * 1024 * 1024, 5)
            .Build()
    );

    PrintBanner(BannerType::World);
    LOG_INFO("Starting World Server...");

    try {
        // 1. Establish Database Connection (Auth database for session validation)
        sql::Driver* driver = sql::mariadb::get_driver_instance();
        sql::SQLString url("jdbc:mariadb://127.0.0.1:3306/firelands_auth");
        sql::Properties properties({{"user", "firelands"}, {"password", "firelands"}});
        std::shared_ptr<sql::Connection> authConn(driver->connect(url, properties));
        
        // 2. Establish Character Database Connection
        sql::SQLString charUrl("jdbc:mariadb://127.0.0.1:3306/firelands_characters");
        std::shared_ptr<sql::Connection> charConn(driver->connect(charUrl, properties));
        
        // 3. Initialize Repositories and Services
        auto accountRepo = std::make_shared<MySqlAccountRepository>(authConn);
        auto authService = std::make_shared<AuthService>(accountRepo);

        auto charRepo = std::make_shared<MySqlCharacterRepository>(charConn);
        auto charService = std::make_shared<CharacterService>(charRepo);

        // TODO: Database Connections (World)
        // TODO: Load Configuration using YAML

        auto sessionFactory = [authService, charService](boost::asio::ip::tcp::socket socket) {
            std::make_shared<WorldSession>(std::move(socket), authService, charService)->Start();
        };

        AsyncNetworkServer worldServer(sessionFactory);

        if (worldServer.Start("0.0.0.0", 8085)) {
            LOG_INFO("World Server listening on port 8085");
            
            // Server Loop
            while (true) {
                worldServer.Update();
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        } else {
            LOG_CRITICAL("Failed to start World Server.");
            Logger::Shutdown();
            return 1;
        }

    } catch (std::exception& e) {
        LOG_CRITICAL("Fatal error: {}", e.what());
        Logger::Shutdown();
        return 1;
    }

    Logger::Shutdown();
    return 0;
}
