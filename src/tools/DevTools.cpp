#include <vector>
#include <string>
#include <iostream>
#include <application/services/SRPService.h>
#include <infrastructure/persistence/MySqlAccountRepository.h>
#include <infrastructure/persistence/MySqlRealmRepository.h>
#include <infrastructure/persistence/DatabaseService.h>
#include <shared/Banner.h>
#include <shared/Logger.h>
#include <shared/Config.h>

using namespace Firelands;

void PrintUsage(const char* progName) {
    LOG_ERROR("Usage:");
    LOG_ERROR("  {} account <username> <password> [email] [expansion (0-4)]", progName);
    LOG_ERROR("  {} realm <id> <name> <address> <port> [icon] [timezone] [secLevel] [population]", progName);
}

int main(int argc, char** argv) {
    Logger::Init(
        LoggerBuilder()
            .WithName("dev-tools")
            .WithConsole(true)
            .WithConsoleLevel(LogLevel::Info)
            .Build()
    );

    PrintBanner(BannerType::Tools);
    LOG_INFO("--- Firelands DevTools ---");

    if (argc < 2) {
        PrintUsage(argv[0]);
        Logger::Shutdown();
        return 1;
    }

    if (!Config::Instance().Load("authserver.yaml")) {
        LOG_WARN("Could not load authserver.yaml, using defaults...");
    }

    std::string command = argv[1];

    try {
        std::string host = Config::Instance().GetNested<std::string>({"Database", "Auth", "Host"}, "127.0.0.1");
        std::string user = Config::Instance().GetNested<std::string>({"Database", "Auth", "User"}, "firelands");
        std::string pass = Config::Instance().GetNested<std::string>({"Database", "Auth", "Password"}, "firelands");
        std::string db = Config::Instance().GetNested<std::string>({"Database", "Auth", "Database"}, "firelands_auth");

        LOG_INFO("Connecting to database...");
        DatabaseService dbService("jdbc:mariadb://" + host + ":3306/" + db, user, pass);
        auto conn = dbService.CreateConnection();

        if (command == "account") {
            if (argc < 4) {
                PrintUsage(argv[0]);
                Logger::Shutdown(); return 1;
            }
            std::string user = argv[2];
            std::string pass = argv[3];
            std::string email = (argc >= 5) ? argv[4] : (user + "@firelands.com");
            uint8 expansion = (argc >= 6) ? static_cast<uint8>(std::stoi(argv[5])) : 4;

            auto accountRepo = std::make_shared<MySqlAccountRepository>(conn);

            if (accountRepo->FindByUsername(user)) {
                LOG_WARN("Account '{}' already exists. Overwriting...", user);
                accountRepo->DeleteByUsername(user);
            }

            SRPData srpData = SRPService::GenerateVerifier(user, pass);
            Account acc;
            acc.username = user;
            acc.email = email;
            acc.salt = srpData.salt;
            acc.verifier = srpData.verifier;
            acc.expansion = expansion;

            LOG_INFO("Inserting account into database...");
            accountRepo->Create(acc);
            LOG_INFO("Account created successfully.");
        } else if (command == "realm") {
            if (argc < 6) {
                PrintUsage(argv[0]);
                Logger::Shutdown(); return 1;
            }
            uint32_t id = std::stoul(argv[2]);
            std::string name = argv[3];
            std::string address = argv[4];
            uint16_t port = static_cast<uint16_t>(std::stoul(argv[5]));
            uint8_t icon = (argc >= 7) ? static_cast<uint8_t>(std::stoul(argv[6])) : 0;
            uint8_t timezone = (argc >= 8) ? static_cast<uint8_t>(std::stoul(argv[7])) : 1;
            uint8_t secLevel = (argc >= 9) ? static_cast<uint8_t>(std::stoul(argv[8])) : 0;
            float population = (argc >= 10) ? std::stof(argv[9]) : 0.0f;

            auto realmRepo = std::make_shared<MySqlRealmRepository>(conn);

            if (realmRepo->FindById(id)) {
                LOG_WARN("Realm ID '{}' already exists. Overwriting...", id);
                realmRepo->DeleteById(id);
            }

            Realm realm(id, name, address, port, icon, timezone, secLevel, population);
            LOG_INFO("Inserting realm into database...");
            realmRepo->Create(realm);
            LOG_INFO("Realm created successfully.");
        } else {
            LOG_ERROR("Unknown command: {}", command);
            PrintUsage(argv[0]);
        }

    } catch (std::exception& e) {
        LOG_CRITICAL("Error: {}", e.what());
        Logger::Shutdown();
        return 1;
    }

    Logger::Shutdown();
    return 0;
}
