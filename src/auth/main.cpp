#include <shared/Banner.h>
#include <shared/Logger.h>
#include <infrastructure/persistence/MySqlAccountRepository.h>
#include <infrastructure/persistence/MySqlRealmRepository.h>
#include <infrastructure/network/sessions/AuthSession.h>
#include <infrastructure/network/asio/AsyncNetworkServer.h>
#include <infrastructure/network/rest/RestAuthServer.h>
#include <infrastructure/persistence/MemoryWebSessionRepository.h>
#include <application/services/WebSessionService.h>

#include <application/services/AuthService.h>
#include <application/services/RealmListService.h>
#include <conncpp.hpp>
#include <thread>
#include <chrono>

using namespace Firelands;

int main() {
    // Initialize logging: console (Info+) and rotating file (Debug+)
    Logger::Init(
        LoggerBuilder()
            .WithName("firelands-auth")
            .WithConsole(true)
            .WithConsoleLevel(LogLevel::Info)
            .WithFile(true, "logs/firelands-auth.log")
            .WithFileLevel(LogLevel::Debug)
            .WithRotatingFile(10 * 1024 * 1024, 5)
            .Build()
    );

    PrintBanner(BannerType::Auth);
    LOG_INFO("Starting Authentication Server...");

    try {
        // 1. Establish Database Connection
        // Pre-requisite: MariaDB docker container must be running
        sql::Driver* driver = sql::mariadb::get_driver_instance();
        sql::SQLString url("jdbc:mariadb://127.0.0.1:3306/firelands_auth");
        sql::Properties properties({{"user", "firelands"}, {"password", "firelands"}});

        std::shared_ptr<sql::Connection> conn(driver->connect(url, properties));
        LOG_INFO("Database connection established.");

        // 2. Initialize Repositories
        auto accountRepo = std::make_shared<MySqlAccountRepository>(conn);
        auto realmRepo = std::make_shared<MySqlRealmRepository>(conn);

        // 3. Initialize Services
        auto authService = std::make_shared<AuthService>(accountRepo);
        auto realmService = std::make_shared<RealmListService>(realmRepo);
        
        // Initialize Web Session Service for REST API
        auto webSessionRepo = std::make_shared<MemoryWebSessionRepository>();
        auto webSessionService = std::make_shared<WebSessionService>(webSessionRepo);


        // 4. Initialize Network Layer
        auto sessionFactory = [authService, realmService](boost::asio::ip::tcp::socket socket) {
            std::make_shared<AuthSession>(std::move(socket), authService, realmService)->Start();
        };
        AsyncNetworkServer authServer(sessionFactory);

        if (authServer.Start("0.0.0.0", 3724)) {
            LOG_INFO("Authentication Server listening on port 3724");

            // 5. Initialize REST API
            RestAuthServer restServer(authService, webSessionService, "0.0.0.0", 8081);

            if (restServer.Start()) {
                LOG_INFO("REST Authentication API listening on port 8081");
            }

            // Server Loop
            while (true) {
                authServer.Update();
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        } else {
            LOG_CRITICAL("Failed to start Authentication Server.");
            Logger::Shutdown();
            return 1;
        }

    } catch (sql::SQLException& e) {
        LOG_CRITICAL("Database error: {}", e.what());
        LOG_ERROR("Please ensure Docker is running and the database is initialized.");
        Logger::Shutdown();
        return 1;
    } catch (std::exception& e) {
        LOG_CRITICAL("Fatal error: {}", e.what());
        Logger::Shutdown();
        return 1;
    }

    Logger::Shutdown();
    return 0;
}

