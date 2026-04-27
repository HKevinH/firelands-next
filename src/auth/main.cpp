#include <shared/Banner.h>
#include <shared/Logger.h>
#include <infrastructure/persistence/MySqlAccountRepository.h>
#include <infrastructure/persistence/MySqlRealmRepository.h>
#include <infrastructure/network/sessions/AuthSession.h>
#include <infrastructure/network/asio/AsyncNetworkServer.h>
#include <infrastructure/network/rest/RestAuthServer.h>
#include <infrastructure/persistence/MemoryWebSessionRepository.h>
#include <application/services/WebSessionService.h>

#include <infrastructure/persistence/DatabaseMigrator.h>
#include <application/services/AuthService.h>
#include <application/services/RealmListService.h>
#include <shared/Config.h>
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

    PrintBanner(BannerType::Auth, true);
    LOG_INFO("Starting Authentication Server...");

    if (!Config::Instance().Load("authserver.yaml")) {
        LOG_WARN("Could not load authserver.yaml, using defaults...");
    }

    try {
        // 1. Database Migration / Validation
        std::string dbUser = Config::Instance().GetNested<std::string>({"Database", "User"}, "firelands");
        std::string dbPass = Config::Instance().GetNested<std::string>({"Database", "Password"}, "firelands");
        
        std::string host = Config::Instance().GetNested<std::string>({"Database", "Auth", "Host"}, "127.0.0.1");
        std::string port = Config::Instance().GetNested<std::string>({"Database", "Auth", "Port"}, "3306");
        std::string db = Config::Instance().GetNested<std::string>({"Database", "Auth", "Database"}, "firelands_auth");

        // Run automatic schema validation/creation
        DatabaseMigrator::MigrateDirectory(host, port, dbUser, dbPass, "sql");

        // 2. Establish Database Connection
        sql::Driver* driver = sql::mariadb::get_driver_instance();
        sql::SQLString url("jdbc:mariadb://" + host + ":" + port + "/" + db);
        sql::Properties properties({{"user", dbUser}, {"password", dbPass}});

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

        std::string bindIp = Config::Instance().GetNested<std::string>({"Network", "BindAddress"}, "0.0.0.0");
        int netPort = Config::Instance().GetNested<int>({"Network", "Port"}, 3724);

        if (authServer.Start(bindIp, netPort)) {
            LOG_INFO("Authentication Server listening on {}:{}", bindIp, netPort);
            // 5. Initialize REST API
            int restPort = Config::Instance().GetNested<int>({"Network", "RestPort"}, 8081);

            RestAuthServer restServer(authService, webSessionService, "0.0.0.0", restPort);

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

