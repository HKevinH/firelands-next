#include <shared/Banner.h>
#include <shared/Logger.h>
#include <infrastructure/network/asio/AsyncNetworkServer.h>
#include <infrastructure/network/sessions/WorldSession.h>
#include <infrastructure/persistence/MySqlAccountRepository.h>
#include <infrastructure/persistence/MySqlCharacterRepository.h>
#include <infrastructure/persistence/DatabaseMigrator.h>
#include <application/services/CharacterService.h>
#include <shared/Config.h>
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

    if (!Config::Instance().Load("worldserver.yaml")) {
        LOG_ERROR("Could not load worldserver.yaml, using defaults or exiting...");
        // In a real scenario, we might want to exit here
    }

    try {
        // General DB Setup
        std::string dbUser = Config::Instance().GetNested<std::string>({"Database", "User"}, "firelands");
        std::string dbPass = Config::Instance().GetNested<std::string>({"Database", "Password"}, "firelands");

        // 1. Database Migration / Validation
        std::string authHost = Config::Instance().GetNested<std::string>({"Database", "Auth", "Host"}, "127.0.0.1");
        std::string authPort = Config::Instance().GetNested<std::string>({"Database", "Auth", "Port"}, "3306");
        
        std::string charHost = Config::Instance().GetNested<std::string>({"Database", "Characters", "Host"}, "127.0.0.1");
        std::string charPort = Config::Instance().GetNested<std::string>({"Database", "Characters", "Port"}, "3306");

        // Validate all required schemas
        DatabaseMigrator::Migrate(authHost, authPort, dbUser, dbPass, "sql/auth_schema.sql");
        DatabaseMigrator::Migrate(charHost, charPort, dbUser, dbPass, "sql/characters_schema.sql");
        DatabaseMigrator::Migrate(charHost, charPort, dbUser, dbPass, "sql/world_schema.sql");

        // 2. Establish Database Connection (Auth database for session validation)
        std::string authDb = Config::Instance().GetNested<std::string>({"Database", "Auth", "Database"}, "firelands_auth");

        sql::Driver* driver = sql::mariadb::get_driver_instance();
        sql::Properties properties({{"user", dbUser}, {"password", dbPass}});
        
        sql::SQLString authUrl("jdbc:mariadb://" + authHost + ":" + authPort + "/" + authDb);
        std::shared_ptr<sql::Connection> authConn(driver->connect(authUrl, properties));
        
        // 3. Establish Character Database Connection
        std::string charDb = Config::Instance().GetNested<std::string>({"Database", "Characters", "Database"}, "firelands_characters");

        sql::SQLString charUrl("jdbc:mariadb://" + charHost + ":" + charPort + "/" + charDb);
        std::shared_ptr<sql::Connection> charConn(driver->connect(charUrl, properties));
        
        // 4. Initialize Repositories and Services
        auto accountRepo = std::make_shared<MySqlAccountRepository>(authConn);
        auto authService = std::make_shared<AuthService>(accountRepo);

        auto charRepo = std::make_shared<MySqlCharacterRepository>(charConn);
        auto charService = std::make_shared<CharacterService>(charRepo);

        auto sessionFactory = [authService, charService](boost::asio::ip::tcp::socket socket) {
            std::make_shared<WorldSession>(std::move(socket), authService, charService)->Start();
        };

        AsyncNetworkServer worldServer(sessionFactory);

        std::string bindIp = Config::Instance().GetNested<std::string>({"Network", "BindAddress"}, "0.0.0.0");
        int port = Config::Instance().GetNested<int>({"Network", "Port"}, 8085);

        if (worldServer.Start(bindIp, port)) {
            LOG_INFO("World Server listening on {}:{}", bindIp, port);
            
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
