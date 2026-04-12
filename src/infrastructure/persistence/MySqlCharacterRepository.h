#ifndef FIRELANDS_INFRASTRUCTURE_PERSISTENCE_MYSQL_CHARACTER_REPOSITORY_H
#define FIRELANDS_INFRASTRUCTURE_PERSISTENCE_MYSQL_CHARACTER_REPOSITORY_H

#include <domain/repositories/ICharacterRepository.h>
#include <shared/Common.h>
#include <conncpp.hpp>
#include <memory>

namespace Firelands {

    class MySqlCharacterRepository : public ICharacterRepository {
    public:
        explicit MySqlCharacterRepository(std::shared_ptr<sql::Connection> connection);

        std::vector<std::shared_ptr<Character>> GetCharactersByAccount(uint32_t accountId) override;
        bool CreateCharacter(const Character& character) override;
        bool DeleteCharacter(uint32_t guid, uint32_t accountId) override;
        bool IsNameAvailable(const std::string& name) override;

    private:
        std::shared_ptr<sql::Connection> _connection;
    };

} // namespace Firelands

#endif // FIRELANDS_INFRASTRUCTURE_PERSISTENCE_MYSQL_CHARACTER_REPOSITORY_H
