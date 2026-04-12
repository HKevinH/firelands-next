#pragma once

#include <shared/Common.h>
#include <string>

namespace Firelands {

class Character {
public:
    Character(uint32 guid, uint32 account, std::string name, uint8 race, uint8 klass, uint8 gender,
              uint8 skin, uint8 face, uint8 hairStyle, uint8 hairColor, uint8 facialHair,
              uint8 level, uint16 zoneId, uint16 mapId, float x, float y, float z,
              uint32 guildId, uint32 characterFlags, uint32 customizationFlags, bool firstLogin)
        : m_guid(guid), m_account(account), m_name(std::move(name)), m_race(race), m_klass(klass), m_gender(gender),
          m_skin(skin), m_face(face), m_hairStyle(hairStyle), m_hairColor(hairColor), m_facialHair(facialHair),
          m_level(level), m_zoneId(zoneId), m_mapId(mapId), m_x(x), m_y(y), m_z(z),
          m_guildId(guildId), m_characterFlags(characterFlags), m_customizationFlags(customizationFlags), m_firstLogin(firstLogin) {}

    uint32 GetGuid() const { return m_guid; }
    uint32 GetAccount() const { return m_account; }
    const std::string& GetName() const { return m_name; }
    uint8 GetRace() const { return m_race; }
    uint8 GetClass() const { return m_klass; }
    uint8 GetGender() const { return m_gender; }
    uint8 GetSkin() const { return m_skin; }
    uint8 GetFace() const { return m_face; }
    uint8 GetHairStyle() const { return m_hairStyle; }
    uint8 GetHairColor() const { return m_hairColor; }
    uint8 GetFacialHair() const { return m_facialHair; }
    uint8 GetLevel() const { return m_level; }
    uint16 GetZoneId() const { return m_zoneId; }
    uint16 GetMapId() const { return m_mapId; }
    float GetX() const { return m_x; }
    float GetY() const { return m_y; }
    float GetZ() const { return m_z; }
    uint32 GetGuildId() const { return m_guildId; }
    uint32 GetCharacterFlags() const { return m_characterFlags; }
    uint32 GetCustomizationFlags() const { return m_customizationFlags; }
    bool IsFirstLogin() const { return m_firstLogin; }

private:
    uint32 m_guid;
    uint32 m_account;
    std::string m_name;
    uint8 m_race;
    uint8 m_klass;
    uint8 m_gender;
    uint8 m_skin;
    uint8 m_face;
    uint8 m_hairStyle;
    uint8 m_hairColor;
    uint8 m_facialHair;
    uint8 m_level;
    uint16 m_zoneId;
    uint16 m_mapId;
    float m_x;
    float m_y;
    float m_z;
    uint32 m_guildId;
    uint32 m_characterFlags;
    uint32 m_customizationFlags;
    bool m_firstLogin;
};

} // namespace Firelands
