#ifndef FIRELANDS_SHARED_NETWORK_UPDATE_FIELDS_H
#define FIRELANDS_SHARED_NETWORK_UPDATE_FIELDS_H

#include <shared/Common.h>

namespace Firelands {

    enum ObjectFields {
        OBJECT_FIELD_GUID = 0x0000,
        OBJECT_FIELD_TYPE = 0x0002,
        OBJECT_FIELD_ENTRY = 0x0003,
        OBJECT_FIELD_SCALE_X = 0x0004,
        OBJECT_END = 0x0005
    };

    enum UnitFields {
        UNIT_FIELD_CHARM = OBJECT_END + 0,
        UNIT_FIELD_SUMMON = OBJECT_END + 2,
        UNIT_FIELD_CHARMEDBY = OBJECT_END + 4,
        UNIT_FIELD_SUMMONEDBY = OBJECT_END + 6,
        UNIT_FIELD_CREATEDBY = OBJECT_END + 8,
        UNIT_FIELD_TARGET = OBJECT_END + 10,
        UNIT_FIELD_CHANNEL_OBJECT = OBJECT_END + 12,
        UNIT_FIELD_CHANNEL_SPELL = OBJECT_END + 14,
        UNIT_FIELD_BYTES_0 = OBJECT_END + 15,
        UNIT_FIELD_HEALTH = OBJECT_END + 16,
        UNIT_FIELD_MAXHEALTH = OBJECT_END + 20,
        UNIT_FIELD_LEVEL = OBJECT_END + 25,
        UNIT_END = OBJECT_END + 160 // Very rough estimate for 4.3.4
    };

    enum PlayerFields {
        PLAYER_FIELD_SELECTION = UNIT_END + 0,
        // ... many more
        PLAYER_END = UNIT_END + 1000
    };

} // namespace Firelands

#endif
