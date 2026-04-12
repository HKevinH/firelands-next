#pragma once

#include <domain/repositories/ICharacterRepository.h>
#include <shared/Common.h>
#include <memory>
#include <vector>

namespace Firelands {

class CharacterService {
public:
    explicit CharacterService(std::shared_ptr<ICharacterRepository> repository)
        : m_repository(std::move(repository)) {}

    std::vector<std::shared_ptr<Character>> GetCharactersForAccount(uint32 accountId) {
        return m_repository->GetCharactersByAccount(accountId);
    }

    bool CreateCharacter(uint32 accountId, std::string name, uint8 race, uint8 klass, uint8 gender,
                         uint8 skin, uint8 face, uint8 hairStyle, uint8 hairColor, uint8 facialHair) {
        
        if (!m_repository->IsNameAvailable(name)) {
            return false;
        }

        // Logic for default spawn location, etc. should ideally be in domain or config
        Character newChar(0, accountId, name, race, klass, gender, skin, face, hairStyle, hairColor, facialHair,
                          1, 0, 0, 0.0f, 0.0f, 0.0f, 0, 0, 0, true);
        
        return m_repository->CreateCharacter(newChar);
    }

    bool DeleteCharacter(uint32 guid, uint32 accountId) {
        return m_repository->DeleteCharacter(guid, accountId);
    }

private:
    std::shared_ptr<ICharacterRepository> m_repository;
};

} // namespace Firelands
