#pragma once

#include <shared/Common.h>

namespace Firelands {

    enum TypeID {
        TYPEID_OBJECT        = 0,
        TYPEID_ITEM          = 1,
        TYPEID_CONTAINER     = 2,
        TYPEID_UNIT          = 3,
        TYPEID_PLAYER        = 4,
        TYPEID_GAMEOBJECT    = 5,
        TYPEID_DYNAMICOBJECT = 6,
        TYPEID_CORPSE        = 7,
        TYPEID_AREATRIGGER   = 8
    };

    enum UpdateType {
        UPDATETYPE_VALUES            = 0,
        UPDATETYPE_MOVEMENT          = 1,
        UPDATETYPE_CREATE_OBJECT     = 2,
        UPDATETYPE_CREATE_OBJECT2    = 3,
        UPDATETYPE_OUT_OF_RANGE_OBJECTS = 4,
        UPDATETYPE_NEAR_OBJECTS      = 5
    };

    // Enumerations for UpdateFields.h matching Cataclysm 4.3.4
    enum ObjectFields {
        OBJECT_FIELD_GUID = 0x0000,
        OBJECT_FIELD_DATA = 0x0002,
        OBJECT_FIELD_TYPE = 0x0004,
        OBJECT_FIELD_ENTRY = 0x0005,
        OBJECT_FIELD_SCALE_X = 0x0006,
        OBJECT_END = 0x0008
    };

    enum UnitFields {
        UNIT_FIELD_CHARM = OBJECT_END + 0x0000,
        UNIT_FIELD_SUMMON = OBJECT_END + 0x0002,
        UNIT_FIELD_CHARMEDBY = OBJECT_END + 0x0006,
        UNIT_FIELD_SUMMONEDBY = OBJECT_END + 0x0008,
        UNIT_FIELD_CREATEDBY = OBJECT_END + 0x000A,
        UNIT_FIELD_TARGET = OBJECT_END + 0x000C,
        UNIT_FIELD_CHANNEL_OBJECT = OBJECT_END + 0x000E,
        UNIT_FIELD_BYTES_0 = OBJECT_END + 0x0011,
        UNIT_FIELD_HEALTH = OBJECT_END + 0x0012,
        UNIT_FIELD_MAXHEALTH = OBJECT_END + 0x0018,
        UNIT_FIELD_LEVEL = OBJECT_END + 0x0028,
        UNIT_FIELD_FACTIONTEMPLATE = OBJECT_END + 0x0033,
        UNIT_FIELD_DISPLAYID = OBJECT_END + 0x0035,
        UNIT_FIELD_FLAGS = OBJECT_END + 0x0038,
        UNIT_FIELD_FLAGS_2 = OBJECT_END + 0x0039,
        UNIT_END = OBJECT_END + 0x00B0 // Adjusted for typical 4.3.4 struct
    };

    enum PlayerFields {
        PLAYER_FIELD_SELECTION = UNIT_END + 0,
        PLAYER_END = UNIT_END + 0x04D6
    };

} // namespace Firelands
