#pragma once

#include <domain/models/Character.h>
#include <vector>
#include <memory>
#include <optional>

namespace Firelands {

class ICharacterRepository {
public:
    virtual ~ICharacterRepository() = default;

    virtual std::vector<std::shared_ptr<Character>> GetCharactersByAccount(uint32_t accountId) = 0;
    virtual bool CreateCharacter(const Character& character) = 0;
    virtual bool DeleteCharacter(uint32_t guid, uint32_t accountId) = 0;
    virtual bool IsNameAvailable(const std::string& name) = 0;
};

} // namespace Firelands
