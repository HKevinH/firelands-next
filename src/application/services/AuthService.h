#ifndef FIRELANDS_APPLICATION_SERVICES_AUTH_SERVICE_H
#define FIRELANDS_APPLICATION_SERVICES_AUTH_SERVICE_H

#include <domain/repositories/IAccountRepository.h>
#include <application/services/SRPService.h>
#include <shared/Crypto.h>
#include <memory>

namespace Firelands {

    class AuthService {
    public:
        explicit AuthService(std::shared_ptr<IAccountRepository> accountRepo)
            : _accountRepo(std::move(accountRepo)) {}


        std::optional<Account> FindAccount(const std::string& username) {
            return _accountRepo->FindByUsername(Crypto::ToUpper(username));
        }

        bool VerifyCredentials(const std::string& username, const std::string& password) {
            auto account = FindAccount(username);
            if (!account) {
                return false;
            }

            return SRPService::VerifyPassword(username, password, account->salt, account->verifier);
        }

        void CreateSession(uint32 accountId, const std::vector<uint8_t>& sessionKey) {
            _accountRepo->CreateSession(accountId, sessionKey);
        }

        std::vector<uint8_t> GetSessionKey(uint32 accountId) {
            return _accountRepo->GetSessionKey(accountId);
        }

        bool ValidateSession(uint32 accountId) {
            // Formal validation: Check if account exists and has a session key
            // Future: Check expiration, IP matches, etc.
            auto key = _accountRepo->GetSessionKey(accountId);
            return !key.empty();
        }

    private:
        std::shared_ptr<IAccountRepository> _accountRepo;
    };

} // namespace Firelands

#endif // FIRELANDS_APPLICATION_SERVICES_AUTH_SERVICE_H
