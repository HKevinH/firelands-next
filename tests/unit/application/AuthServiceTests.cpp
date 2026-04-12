#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <application/services/AuthService.h>
#include <domain/repositories/IAccountRepository.h>

using namespace Firelands;
using namespace testing;

class MockAccountRepository : public IAccountRepository {
public:
    MOCK_METHOD(std::optional<Account>, FindByUsername, (const std::string&), (override));
    MOCK_METHOD(void, Create, (const Account&), (override));
    MOCK_METHOD(void, Update, (const Account&), (override));
    MOCK_METHOD(void, DeleteByUsername, (const std::string&), (override));
    MOCK_METHOD(void, CreateSession, (uint32, const std::vector<uint8_t>&), (override));
    MOCK_METHOD(std::vector<uint8_t>, GetSessionKey, (uint32), (override));
};

TEST(AuthServiceTests, Login_WithValidCredentials_ReturnsTrue) {
    auto repo = std::make_shared<MockAccountRepository>();
    AuthService service(repo);

    std::string username = "TESTUSER";
    std::string password = "password";
    
    // Generate valid SRP verifier for this user/pass
    auto srpData = SRPService::GenerateVerifier(username, password);

    Account account;
    account.username = username;
    account.salt = srpData.salt;
    account.verifier = srpData.verifier;

    EXPECT_CALL(*repo, FindByUsername(username))
        .WillOnce(Return(account));

    EXPECT_TRUE(service.VerifyCredentials(username, password));
}

TEST(AuthServiceTests, Login_WithInvalidPassword_ReturnsFalse) {
    auto repo = std::make_shared<MockAccountRepository>();
    AuthService service(repo);

    std::string username = "TESTUSER";
    std::string password = "password";
    
    auto srpData = SRPService::GenerateVerifier(username, password);

    Account account;
    account.username = username;
    account.salt = srpData.salt;
    account.verifier = srpData.verifier;

    EXPECT_CALL(*repo, FindByUsername(username))
        .WillOnce(Return(account));

    EXPECT_FALSE(service.VerifyCredentials(username, "wrongpassword"));
}
