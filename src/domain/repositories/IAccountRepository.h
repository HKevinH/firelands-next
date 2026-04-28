#ifndef FIRELANDS_DOMAIN_REPOSITORIES_IACCOUNT_REPOSITORY_H
#define FIRELANDS_DOMAIN_REPOSITORIES_IACCOUNT_REPOSITORY_H

#include <optional>
#include <shared/Common.h>
#include <string>
#include <vector>

namespace Firelands {

struct Account {
  uint32 id;
  std::string username;
  std::string email;
  std::vector<uint8_t> salt;
  std::vector<uint8_t> verifier;
  uint8 expansion;
};

class IAccountRepository {
public:
  virtual ~IAccountRepository() = default;

  virtual std::optional<Account>
  FindByUsername(const std::string &username) = 0;
  virtual void Create(const Account &account) = 0;
  virtual void Update(const Account &account) = 0;
  virtual void DeleteByUsername(const std::string &username) = 0;

  virtual void CreateSession(uint32 accountId,
                             const std::vector<uint8_t> &sessionKey) = 0;
  virtual std::vector<uint8_t> GetSessionKey(uint32 accountId) = 0;
};

} // namespace Firelands

#endif // FIRELANDS_DOMAIN_REPOSITORIES_IACCOUNT_REPOSITORY_H
