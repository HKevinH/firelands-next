#ifndef FIRELANDS_SHARED_NETWORK_WORLD_OPCODES_H
#define FIRELANDS_SHARED_NETWORK_WORLD_OPCODES_H

#include <shared/Common.h>

namespace Firelands {

    enum WorldOpcode : uint32 {
        SMSG_AUTH_CHALLENGE              = 0x01EC,
        CMSG_AUTH_SESSION                = 0x01EE,
        SMSG_AUTH_RESPONSE               = 0x03BC,

        CMSG_CHAR_ENUM                   = 0x0037,
        SMSG_CHAR_ENUM                   = 0x05E5,
        CMSG_CHAR_CREATE                 = 0x0036,
        SMSG_CHAR_CREATE                 = 0x003A,
        CMSG_CHAR_DELETE                 = 0x0038,
        SMSG_CHAR_DELETE                 = 0x003C,

        CMSG_PLAYER_LOGIN                = 0x0113,
        SMSG_LOGIN_VERIFY_WORLD          = 0x01CC,
        SMSG_TIME_SYNC_REQ               = 0x015B,
        SMSG_TUTORIAL_FLAGS              = 0x011F,
        SMSG_MOTD                        = 0x0140,
        SMSG_UPDATE_OBJECT               = 0x0112,
        SMSG_ACCOUNT_DATA_TIMES          = 0x0157,
        SMSG_INITIAL_SPELLS              = 0x0110,
        SMSG_ACTION_BUTTONS              = 0x012B,
    };

} // namespace Firelands

#endif
