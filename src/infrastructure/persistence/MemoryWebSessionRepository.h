#ifndef FIRELANDS_INFRASTRUCTURE_PERSISTENCE_MEMORY_WEB_SESSION_REPOSITORY_H
#define FIRELANDS_INFRASTRUCTURE_PERSISTENCE_MEMORY_WEB_SESSION_REPOSITORY_H

#include <domain/repositories/IWebSessionRepository.h>
#include <mutex>
#include <unordered_map>

namespace Firelands {

class MemoryWebSessionRepository : public IWebSessionRepository {
public:
  void Save(const WebSession &session) override {
    std::lock_guard<std::mutex> lock(_mutex);
    _sessions[session.token] = session;
  }

  std::optional<WebSession> FindByToken(const std::string &token) override {
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _sessions.find(token);
    if (it != _sessions.end()) {
      return it->second;
    }
    return std::nullopt;
  }

  void DeleteByToken(const std::string &token) override {
    std::lock_guard<std::mutex> lock(_mutex);
    _sessions.erase(token);
  }

  void DeleteExpired() override {
    std::lock_guard<std::mutex> lock(_mutex);
    auto now = std::chrono::system_clock::now();
    for (auto it = _sessions.begin(); it != _sessions.end();) {
      if (it->second.expiresAt < now) {
        it = _sessions.erase(it);
      } else {
        ++it;
      }
    }
  }

private:
  std::unordered_map<std::string, WebSession> _sessions;
  std::mutex _mutex;
};

} // namespace Firelands

#endif // FIRELANDS_INFRASTRUCTURE_PERSISTENCE_MEMORY_WEB_SESSION_REPOSITORY_H
