#include <application/services/WebSessionService.h>
#include <infrastructure/network/asio/AsyncNetworkServer.h>
#include <infrastructure/network/rest/RestAuthServer.h>
#include <infrastructure/network/sessions/AuthSession.h>
#include <infrastructure/persistence/MemoryWebSessionRepository.h>
#include <infrastructure/persistence/MySqlAccountRepository.h>
#include <infrastructure/persistence/MySqlRealmRepository.h>
#include <shared/Banner.h>
#include <shared/Logger.h>

#include <application/services/AuthService.h>
#include <application/services/RealmListService.h>
#include <chrono>
#include <conncpp.hpp>
#include <infrastructure/persistence/DatabaseMigrator.h>
#include <shared/Config.h>
#include <thread>

using namespace Firelands;

int main() {
  PrintBanner(BannerType::Auth, true);

  // Initialize logging before config load
  Logger::Init(LoggerBuilder()
                   .WithName("firelands-auth")
                   .WithConsole(true)
                   .WithConsoleLevel(LogLevel::Info)
                   .Build());

  auto config = Config::Instance();

  if (!config.Load("authserver.yaml")) {
    LOG_WARN("Could not load authserver.yaml, using defaults...");
  }

  // Update logging with config values if needed
  LogLevel consoleLevel = config.GetNested<LogLevel>({"Log", "Level"}, LogLevel::Info);
  std::string logFile = config.GetNested<std::string>({"Log", "File"}, "logs/firelands-auth.log");

  Logger::Shutdown();
  Logger::Init(LoggerBuilder()
                   .WithName("firelands-auth")
                   .WithConsole(true)
                   .WithConsoleLevel(consoleLevel)
                   .WithFile(true, logFile)
                   .WithFileLevel(LogLevel::Debug)
                   .WithRotatingFile(10 * 1024 * 1024, 5)
                   .Build());

  LOG_INFO("Starting Authentication Server...");

  try {
    // 1. Database Migration / Validation
    std::string dbUser =
        config.GetNested<std::string>({"Database", "User"}, "firelands");
    std::string dbPass =
        config.GetNested<std::string>({"Database", "Password"}, "firelands");

    std::string authUrl = config.GetNested<std::string>(
        {"Database", "Auth", "URI"},
        "jdbc:mariadb://localhost:3306/firelands_auth");

    // Run automatic schema validation/creation
    DatabaseMigrator::MigrateDirectory(authUrl, dbUser, dbPass, "sql");

    // 2. Establish Database Connection
    sql::Driver *driver = sql::mariadb::get_driver_instance();
    sql::Properties properties({{"user", dbUser}, {"password", dbPass}});

    std::shared_ptr<sql::Connection> conn(driver->connect(authUrl, properties));
    LOG_INFO("Database connection established.");

    // 2. Initialize Repositories
    auto accountRepo = std::make_shared<MySqlAccountRepository>(conn);
    auto realmRepo = std::make_shared<MySqlRealmRepository>(conn);

    // 3. Initialize Services
    auto authService = std::make_shared<AuthService>(accountRepo);
    auto realmService = std::make_shared<RealmListService>(realmRepo);

    // Initialize Web Session Service for REST API
    auto webSessionRepo = std::make_shared<MemoryWebSessionRepository>();
    auto webSessionService =
        std::make_shared<WebSessionService>(webSessionRepo);

    // 4. Initialize Network Layer
    auto sessionFactory = [authService,
                           realmService](boost::asio::ip::tcp::socket socket) {
      std::make_shared<AuthSession>(std::move(socket), authService,
                                    realmService)
          ->Start();
    };
    AsyncNetworkServer authServer(sessionFactory);

    std::string bindIp = config.GetNested<std::string>({"Network", "BindAddress"}, "0.0.0.0");
    int netPort = config.GetNested<int>({"Network", "Port"}, 3724);

    if (authServer.Start(bindIp, netPort)) {

      LOG_INFO("Authentication Server listening on {}:{}", bindIp, netPort);
      // 5. Initialize REST API
      std::string restBindIp = config.GetNested<std::string>({"Network", "BindAddress"}, "0.0.0.0");
      int restPort = config.GetNested<int>({"Network", "RestPort"}, 8081);

      RestAuthServer restServer(authService, webSessionService, restBindIp,
                                restPort);

      if (restServer.Start()) {
        LOG_INFO("REST Authentication API listening on {}:{}", bindIp,
                 restPort);
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

  } catch (sql::SQLException &e) {
    LOG_CRITICAL("Database error: {}", e.what());
    LOG_ERROR(
        "Please ensure Docker is running and the database is initialized.");
    Logger::Shutdown();
    return 1;
  } catch (std::exception &e) {
    LOG_CRITICAL("Fatal error: {}", e.what());
    Logger::Shutdown();
    return 1;
  }

  Logger::Shutdown();
  return 0;
}
