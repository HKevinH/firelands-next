#include <gtest/gtest.h>
#include <shared/network/UpdateData.h>
#include <shared/network/WorldPacket.h>
#include <shared/network/UpdateFields.h>

using namespace Firelands;

TEST(UpdateDataTest, BuildCreateObjectPacketForPlayer) {
    UpdateData data;
    
    uint64 guid = 0x1234567890ABCDEF;
    
    MovementInfo move;
    move.x = 100.0f;
    move.y = 200.0f;
    move.z = 300.0f;
    
    std::map<uint16, uint32> fields;
    fields[OBJECT_FIELD_GUID] = (uint32)(guid & 0xFFFFFFFF);
    fields[OBJECT_FIELD_GUID + 1] = (uint32)(guid >> 32);
    
    // Add a dummy CreateObject block
    data.AddCreateObject(guid, TYPEID_PLAYER, move, fields);
    
    WorldPacket packet(SMSG_UPDATE_OBJECT);
    data.Build(packet);
    
    // Check structure
    packet.SetReadPos(0);
    EXPECT_EQ(packet.Read<uint16>(), 0); // MapId
    ASSERT_EQ(packet.Read<uint32>(), 1); // 1 block
    EXPECT_EQ(packet.Read<uint8>(), UPDATETYPE_CREATE_OBJECT2);
}
