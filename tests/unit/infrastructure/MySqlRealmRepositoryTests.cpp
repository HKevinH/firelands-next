#include <gtest/gtest.h>
#include <infrastructure/persistence/MySqlRealmRepository.h>
#include <infrastructure/persistence/DatabaseService.h>
#include <memory>

namespace Firelands {
namespace {

    class MySqlRealmRepositoryTest : public ::testing::Test {
    protected:
        void SetUp() override {
            std::string url = "tcp://127.0.0.1:3306/firelands_auth";
            std::string user = "root";
            std::string pass = "root";

            try {
                DatabaseService dbService(url, user, pass);
                _connection = dbService.CreateConnection();
                _repository = std::make_unique<MySqlRealmRepository>(_connection);
                
                // Cleanup before each test and insert a test realm
                auto stmt = std::shared_ptr<sql::Statement>(_connection->createStatement());
                stmt->execute("DELETE FROM realmlist");
                stmt->execute("INSERT INTO realmlist (id, name, address, port, icon, timezone, allowedSecurityLevel, population) VALUES (1, 'TestRealm', '127.0.0.1', 8085, 0, 1, 0, 0.0)");
            } catch (const std::exception& e) {
                GTEST_SKIP() << "Database connection failed. Is Docker running? Error: " << e.what();
            }
        }

        void TearDown() override {
            if (_connection) {
                auto stmt = std::shared_ptr<sql::Statement>(_connection->createStatement());
                stmt->execute("DELETE FROM realmlist");
            }
        }

        std::shared_ptr<sql::Connection> _connection;
        std::unique_ptr<MySqlRealmRepository> _repository;
    };

    TEST_F(MySqlRealmRepositoryTest, GetRealmsReturnsList) {
        auto realms = _repository->GetRealms();
        
        ASSERT_EQ(realms.size(), 1);
        EXPECT_EQ(realms[0].GetId(), 1);
        EXPECT_EQ(realms[0].GetName(), "TestRealm");
        EXPECT_EQ(realms[0].GetAddress(), "127.0.0.1");
        EXPECT_EQ(realms[0].GetPort(), 8085);
        EXPECT_EQ(realms[0].GetIcon(), 0);
        EXPECT_EQ(realms[0].GetTimezone(), 1);
        EXPECT_EQ(realms[0].GetAllowedSecurityLevel(), 0);
        EXPECT_FLOAT_EQ(realms[0].GetPopulation(), 0.0f);
    }

    TEST_F(MySqlRealmRepositoryTest, GetAllowedSecurityLevelForRealm) {
        auto gate = _repository->GetAllowedSecurityLevelForRealm(1);
        ASSERT_TRUE(gate.has_value());
        EXPECT_EQ(*gate, 0u);
        EXPECT_FALSE(_repository->GetAllowedSecurityLevelForRealm(999).has_value());
    }

} // namespace
} // namespace Firelands
