#ifndef FIRELANDS_DOMAIN_MODELS_WEB_SESSION_H
#define FIRELANDS_DOMAIN_MODELS_WEB_SESSION_H

#include <shared/Common.h>
#include <string>
#include <chrono>

namespace Firelands {

    struct WebSession {
        std::string token;
        uint32 accountId;
        std::chrono::system_clock::time_point expiresAt;

        bool IsExpired() const {
            return std::chrono::system_clock::now() > expiresAt;
        }
    };

} // namespace Firelands

#endif // FIRELANDS_DOMAIN_MODELS_WEB_SESSION_H
