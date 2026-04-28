#ifndef FIRELANDS_DOMAIN_REPOSITORIES_IWEB_SESSION_REPOSITORY_H
#define FIRELANDS_DOMAIN_REPOSITORIES_IWEB_SESSION_REPOSITORY_H

#include <domain/models/WebSession.h>
#include <optional>
#include <string>

namespace Firelands {

class IWebSessionRepository {
public:
  virtual ~IWebSessionRepository() = default;

  virtual void Save(const WebSession &session) = 0;
  virtual std::optional<WebSession> FindByToken(const std::string &token) = 0;
  virtual void DeleteByToken(const std::string &token) = 0;
  virtual void DeleteExpired() = 0;
};

} // namespace Firelands

#endif // FIRELANDS_DOMAIN_REPOSITORIES_IWEB_SESSION_REPOSITORY_H
