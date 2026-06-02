#include <infrastructure/network/sessions/WorldSession.h>

#include <domain/world/ChannelManager.h>
#include <shared/game/PlayerFactionTeam.h>
#include <shared/network/BitReader.h>
#include <shared/network/WorldOpcodes.h>
#include <shared/network/WorldPacket.h>
#include <shared/Logger.h>

#include <cctype>

namespace Firelands {

namespace {

// SMSG_CHANNEL_NOTIFY notice types (build 15595 / 4.3.4 — same values as 3.3.5).
constexpr uint8 kChatYouJoinedNotice = 0x02;
constexpr uint8 kChatYouLeftNotice = 0x03;

// ChannelFlags (4.3.4 `ChannelFlags` enum). The client needs these on a
// server-pushed YOU_JOINED to slot built-in zone channels correctly; wrong or
// zero flags make it silently ignore the join (the YOU_LEFT carries no flags,
// so leaving still works — which is why a zone swap left but never re-joined).
constexpr uint8 kChannelFlagCustom = 0x01;
constexpr uint8 kChannelFlagTrade = 0x02;
constexpr uint8 kChannelFlagNotLfg = 0x04;
constexpr uint8 kChannelFlagGeneral = 0x08;
constexpr uint8 kChannelFlagCity = 0x10;
constexpr uint8 kChannelFlagLfg = 0x20;

// Built-in ChatChannels.dbc ids -> channel flags (matches Channel ctor in a 4.3.4
// core). channelId 0 = custom channel.
uint8 ChannelFlagsForId(uint32 channelId) {
  switch (channelId) {
    case 1:  // General
      return kChannelFlagGeneral | kChannelFlagNotLfg;
    case 2:  // Trade (city-only)
      return kChannelFlagGeneral | kChannelFlagTrade | kChannelFlagCity |
             kChannelFlagNotLfg;
    case 22:  // LocalDefense
    case 23:  // WorldDefense
    case 25:  // GuildRecruitment
      return kChannelFlagGeneral | kChannelFlagNotLfg;
    case 26:  // LookingForGroup
      return kChannelFlagGeneral | kChannelFlagLfg;
    default:
      return channelId != 0 ? (kChannelFlagGeneral | kChannelFlagNotLfg)
                            : kChannelFlagCustom;
  }
}

// SMSG_CHANNEL_NOTIFY is PLAIN byte layout (uint8 type, cstring name, then the
// type-specific tail) — confirmed against a 4.3.4 core (Channel::MakeNotifyPacket
// / MakeYouJoined / MakeYouLeft). Not bit-packed, so safe to emit to the client.
WorldPacket BuildYouJoinedNotify(std::string const &channelName,
                                 uint32 channelId) {
  WorldPacket data(static_cast<uint32>(SMSG_CHANNEL_NOTIFY),
                   1 + channelName.size() + 2 + 4 + 4);
  data.Append<uint8>(kChatYouJoinedNotice);
  data.WriteString(channelName);
  data.Append<uint8>(ChannelFlagsForId(channelId));  // built-in vs custom flags
  data.Append<uint32>(channelId);  // echo the id the client sent on join
  data.Append<uint32>(0);          // padding
  return data;
}

WorldPacket BuildYouLeftNotify(std::string const &channelName,
                               uint32 channelId) {
  WorldPacket data(static_cast<uint32>(SMSG_CHANNEL_NOTIFY),
                   1 + channelName.size() + 1 + 4 + 1);
  data.Append<uint8>(kChatYouLeftNotice);
  data.WriteString(channelName);
  data.Append<uint32>(channelId);
  data.Append<uint8>(0);  // suspended / isConstant
  return data;
}

} // namespace

// City / zone chat channels ("General - Orgrimmar", "Trade - City", ...). The
// CLIENT auto-joins the right channels per zone and sends these opcodes; the
// server tracks membership (ChannelManager) and fans CHAT_MSG_CHANNEL messages out
// to the members (see WorldSessionChatHandlers.cpp).
//
// Wire layout matches a 4.3.4 core (ArkCORE): channelId (uint32), two unused
// bytes, then null-terminated password and channel name. PLAIN, not bit-packed.
// NOTE(verify-wire): if a capture shows bit-packed string lengths, switch the
// string reads to BitReader — a wrong CMSG parse only breaks the join (the packet
// is self-contained), it never corrupts the stream.

void WorldSession::HandleJoinChannel(WorldPacket &packet) {
  if (_playerGuid == 0 || packet.Size() < 6)
    return;
  try {
    // Build 15595 CMSG_JOIN_CHANNEL (verified vs WowPacketParser HandleChannelJoin434):
    // int32 channelId, 1 bit hasVoice, 1 bit joinedByZoneUpdate, 8-bit name length,
    // 8-bit password length, then the name and password bytes. Bit-packed.
    uint32 const channelId = packet.Read<uint32>();
    BitReader br(packet);
    br.ReadBit();  // hasVoice
    br.ReadBit();  // joinedByZoneUpdate
    uint32 const nameLen = br.ReadBits(8);
    uint32 const passLen = br.ReadBits(8);
    if (nameLen == 0 || nameLen > 64 || passLen > 64)
      return;
    std::string const channelName = br.ReadString(nameLen);
    (void)br.ReadString(passLen);  // password (unused in phase 1)
    if (channelName.empty())
      return;

    uint8 const team =
        static_cast<uint8>(FactionSideFromPlayableRace(_playerRace));
    auto const r = ChannelManager::Instance().Join(team, channelName,
                                                   _playerGuid, channelId);
    LOG_DEBUG("[CHANNEL] join guid={} team={} channel='{}' id={} already={}",
              _playerGuid, team, r.displayName, channelId, r.alreadyMember);

    if (!r.alreadyMember) {
      WorldPacket notify = BuildYouJoinedNotify(r.displayName, channelId);
      SendPacket(notify);
    }
  } catch (std::exception const &e) {
    LOG_DEBUG("[CHANNEL] join parse failed guid={} err={}", _playerGuid,
              e.what());
  }
}

void WorldSession::HandleLeaveChannel(WorldPacket &packet) {
  if (_playerGuid == 0 || packet.Size() < 5)
    return;
  try {
    // Best-effort bit-packed leave (mirrors join): int32 channelId, 8-bit name
    // length, name. NOTE(verify-wire): logout + the zone-swap already clean up
    // membership, so an off parse here is low impact.
    uint32 const channelId = packet.Read<uint32>();
    BitReader br(packet);
    uint32 const nameLen = br.ReadBits(8);
    if (nameLen == 0 || nameLen > 64)
      return;
    std::string const channelName = br.ReadString(nameLen);
    if (channelName.empty())
      return;

    uint8 const team =
        static_cast<uint8>(FactionSideFromPlayableRace(_playerRace));
    auto const display =
        ChannelManager::Instance().Leave(team, channelName, _playerGuid);
    LOG_DEBUG("[CHANNEL] leave guid={} channel='{}' wasMember={}", _playerGuid,
              channelName, display.has_value());

    if (display) {
      WorldPacket notify = BuildYouLeftNotify(*display, channelId);
      SendPacket(notify);
    }
  } catch (std::exception const &e) {
    LOG_DEBUG("[CHANNEL] leave parse failed guid={} err={}", _playerGuid,
              e.what());
  }
}

// CMSG_CHAT_CHANNEL_DISPLAY_LIST: client asks for a channel's roster. Reply with
// SMSG_CHANNEL_LIST. Plain byte layout confirmed against a 4.3.4 core
// (Channel::List): uint8(1), cstring name, uint8(channelFlags), uint32(count),
// then per member { uint64 guid, uint8 memberFlags }.
void WorldSession::HandleChannelDisplayList(WorldPacket &packet) {
  if (_playerGuid == 0)
    return;
  try {
    // Best-effort bit-packed: 8-bit name length + name. NOTE(verify-wire): only
    // affects the channel roster UI; an off parse just yields an empty list.
    BitReader br(packet);
    uint32 const nameLen = br.ReadBits(8);
    if (nameLen == 0 || nameLen > 64)
      return;
    std::string const channelName = br.ReadString(nameLen);
    if (channelName.empty())
      return;

    uint8 const team =
        static_cast<uint8>(FactionSideFromPlayableRace(_playerRace));
    auto const members = ChannelManager::Instance().Members(team, channelName);

    WorldPacket data(static_cast<uint32>(SMSG_CHANNEL_LIST),
                     1 + channelName.size() + 1 + 1 + 4 +
                         members.size() * (8 + 1));
    data.Append<uint8>(1);  // channel type / display flag
    data.WriteString(channelName);
    data.Append<uint8>(0);  // channel flags (cosmetic for custom channels)
    data.Append<uint32>(static_cast<uint32>(members.size()));
    for (uint64 const memberGuid : members) {
      data.Append<uint64>(memberGuid);
      data.Append<uint8>(0);  // member flags (0 = normal member)
    }
    SendPacket(data);
    LOG_DEBUG("[CHANNEL] list guid={} channel='{}' members={}", _playerGuid,
              channelName, members.size());
  } catch (std::exception const &e) {
    LOG_DEBUG("[CHANNEL] list parse failed guid={} err={}", _playerGuid,
              e.what());
  }
}

// Server-driven zone channel switch. The 4.3.4 client does not re-join zone
// channels on zone change here, so on a zone change we move the player from
// "<base> - <oldZone>" to "<base> - <newZone>" ourselves. Only channels whose
// name ends in " - <oldZone>" are touched, so non-zone channels ("Trade - City",
// custom channels) are left alone. Reuses the same dbc channelId for the notices.
void WorldSession::UpdateZoneChannels(std::string const &oldZoneName,
                                      std::string const &newZoneName) {
  if (_playerGuid == 0 || oldZoneName.empty() || newZoneName.empty())
    return;
  uint8 const team =
      static_cast<uint8>(FactionSideFromPlayableRace(_playerRace));

  auto const channels =
      ChannelManager::Instance().ChannelsForPlayer(team, _playerGuid);
  for (auto const &ch : channels) {
    // Zone-dependent channels end with the zone name ("Comercio - Azshara",
    // "Defensa local: Azshara", ...). Match the zone name at the end regardless of
    // the localized separator, and require a non-alphanumeric char right before it
    // so we don't catch a custom channel that merely ends in the zone word.
    if (ch.name.size() <= oldZoneName.size())
      continue;
    size_t const pos = ch.name.size() - oldZoneName.size();
    if (ch.name.compare(pos, oldZoneName.size(), oldZoneName) != 0)
      continue;
    if (std::isalnum(static_cast<unsigned char>(ch.name[pos - 1])))
      continue;

    // Keep the prefix + separator, swap only the zone name.
    std::string const newName = ch.name.substr(0, pos) + newZoneName;

    auto const leftDisplay =
        ChannelManager::Instance().Leave(team, ch.name, _playerGuid);
    if (leftDisplay) {
      WorldPacket left = BuildYouLeftNotify(*leftDisplay, ch.channelId);
      SendPacket(left);
    }
    auto const r = ChannelManager::Instance().Join(team, newName, _playerGuid,
                                                   ch.channelId);
    WorldPacket joined = BuildYouJoinedNotify(r.displayName, ch.channelId);
    SendPacket(joined);
    LOG_DEBUG("[CHANNEL] zone-swap guid={} '{}' -> '{}'", _playerGuid, ch.name,
              newName);
  }
}

} // namespace Firelands
