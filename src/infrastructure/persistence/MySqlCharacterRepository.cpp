#include "MySqlCharacterRepository.h"
#include <shared/Logger.h>

namespace Firelands {

    MySqlCharacterRepository::MySqlCharacterRepository(std::shared_ptr<sql::Connection> connection)
        : _connection(std::move(connection)) {}

    std::vector<std::shared_ptr<Character>> MySqlCharacterRepository::GetCharactersByAccount(uint32_t accountId) {
        std::vector<std::shared_ptr<Character>> characters;
        try {
            std::shared_ptr<sql::PreparedStatement> stmnt(
                _connection->prepareStatement(
                    "SELECT guid, account, name, race, class, gender, skin, face, hairStyle, hairColor, facialHair, "
                    "level, zoneId, mapId, x, y, z, guildId, characterFlags, customizationFlags, firstLogin "
                    "FROM characters WHERE account = ?")
            );
            stmnt->setUInt(1, accountId);
            
            std::unique_ptr<sql::ResultSet> res(stmnt->executeQuery());
            
            while (res->next()) {
                characters.push_back(std::make_shared<Character>(
                    res->getUInt("guid"),
                    res->getUInt("account"),
                    std::string(res->getString("name")),
                    static_cast<uint8>(res->getUInt("race")),
                    static_cast<uint8>(res->getUInt("class")),
                    static_cast<uint8>(res->getUInt("gender")),
                    static_cast<uint8>(res->getUInt("skin")),
                    static_cast<uint8>(res->getUInt("face")),
                    static_cast<uint8>(res->getUInt("hairStyle")),
                    static_cast<uint8>(res->getUInt("hairColor")),
                    static_cast<uint8>(res->getUInt("facialHair")),
                    static_cast<uint8>(res->getUInt("level")),
                    static_cast<uint16>(res->getUInt("zoneId")),
                    static_cast<uint16>(res->getUInt("mapId")),
                    res->getFloat("x"),
                    res->getFloat("y"),
                    res->getFloat("z"),
                    res->getUInt("guildId"),
                    res->getUInt("characterFlags"),
                    res->getUInt("customizationFlags"),
                    res->getBoolean("firstLogin")
                ));
            }
        } catch (sql::SQLException& e) {
            LOG_ERROR("Database error in GetCharactersByAccount: {}", e.what());
        }
        return characters;
    }

    bool MySqlCharacterRepository::CreateCharacter(const Character& character) {
        try {
            std::shared_ptr<sql::PreparedStatement> stmnt(
                _connection->prepareStatement(
                    "INSERT INTO characters (account, name, race, class, gender, skin, face, hairStyle, hairColor, facialHair, "
                    "level, zoneId, mapId, x, y, z, guildId, characterFlags, customizationFlags, firstLogin) "
                    "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)")
            );
            
            stmnt->setUInt(1, character.GetAccount());
            stmnt->setString(2, character.GetName());
            stmnt->setUInt(3, character.GetRace());
            stmnt->setUInt(4, character.GetClass());
            stmnt->setUInt(5, character.GetGender());
            stmnt->setUInt(6, character.GetSkin());
            stmnt->setUInt(7, character.GetFace());
            stmnt->setUInt(8, character.GetHairStyle());
            stmnt->setUInt(9, character.GetHairColor());
            stmnt->setUInt(10, character.GetFacialHair());
            stmnt->setUInt(11, character.GetLevel());
            stmnt->setUInt(12, character.GetZoneId());
            stmnt->setUInt(13, character.GetMapId());
            stmnt->setDouble(14, static_cast<double>(character.GetX()));
            stmnt->setDouble(15, static_cast<double>(character.GetY()));
            stmnt->setDouble(16, static_cast<double>(character.GetZ()));
            stmnt->setUInt(17, character.GetGuildId());
            stmnt->setUInt(18, character.GetCharacterFlags());
            stmnt->setUInt(19, character.GetCustomizationFlags());
            stmnt->setBoolean(20, character.IsFirstLogin());
            
            stmnt->executeUpdate();
            return true;
        } catch (sql::SQLException& e) {
            LOG_ERROR("Database error in CreateCharacter: {} (SQLState: {}, ErrorCode: {})", 
                      e.what(), e.getSQLState().c_str(), e.getErrorCode());
            return false;
        }
    }

    bool MySqlCharacterRepository::DeleteCharacter(uint32_t guid, uint32_t accountId) {
        try {
            std::shared_ptr<sql::PreparedStatement> stmnt(
                _connection->prepareStatement("DELETE FROM characters WHERE guid = ? AND account = ?")
            );
            stmnt->setUInt(1, guid);
            stmnt->setUInt(2, accountId);
            stmnt->executeUpdate();
            return true;
        } catch (sql::SQLException& e) {
            LOG_ERROR("Database error in DeleteCharacter: {}", e.what());
            return false;
        }
    }

    bool MySqlCharacterRepository::IsNameAvailable(const std::string& name) {
        try {
            std::shared_ptr<sql::PreparedStatement> stmnt(
                _connection->prepareStatement("SELECT 1 FROM characters WHERE name = ?")
            );
            stmnt->setString(1, name);
            std::unique_ptr<sql::ResultSet> res(stmnt->executeQuery());
            return !res->next();
        } catch (sql::SQLException& e) {
            LOG_ERROR("Database error in IsNameAvailable: {}", e.what());
            return false;
        }
    }

} // namespace Firelands
