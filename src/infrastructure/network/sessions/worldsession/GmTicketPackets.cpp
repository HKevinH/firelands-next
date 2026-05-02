#include "GmTicketPackets.h"
#include <shared/network/WorldOpcodes.h>
#include <ctime>
#include <zlib.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

namespace Firelands {
namespace gm_ticket {

namespace {

constexpr uint64_t kDaySec = 86400;
constexpr size_t kMaxMessageLen = 1999;

} // namespace

float AgeInDays(uint64_t nowUnixSec, uint64_t eventUnixSec) {
  if (nowUnixSec <= eventUnixSec)
    return 0.f;
  return static_cast<float>(nowUnixSec - eventUnixSec) /
         static_cast<float>(kDaySec);
}

void BuildGetTicketHasText(WorldPacket &out, GmTicket const &ticket,
                           std::optional<uint64_t> oldestActiveUpdatedUnix,
                           uint64_t lastQueueChangeUnix) {
  out.Clear();
  out.SetOpcode(SMSG_GM_TICKET_GET_TICKET);
  uint64_t const now = static_cast<uint64_t>(std::time(nullptr));
  uint64_t const updatedSec = ticket.updatedAtUnixMs / 1000ull;
  uint64_t const oldestSec =
      oldestActiveUpdatedUnix ? *oldestActiveUpdatedUnix : updatedSec;

  out.Append<uint32_t>(kWireStatusHasText);
  out.Append<uint32_t>(static_cast<uint32_t>(ticket.id));
  out.WriteString(ticket.message);
  out.Append<uint8_t>(ticket.needMoreHelp ? 1u : 0u);
  out.Append<float>(AgeInDays(now, updatedSec));
  out.Append<float>(AgeInDays(now, oldestSec));
  out.Append<float>(AgeInDays(now, lastQueueChangeUnix));

  uint8_t escalated = kEscalationUnassigned;
  if (ticket.assignedAccountId)
    escalated = kEscalationAssigned;
  out.Append<uint8_t>(std::min(escalated, kEscalationCap));

  uint8_t opened = (ticket.status >= GmTicketStatus::Assigned) ? kOpenedByGmYes
                                                                : kOpenedByGmNot;
  out.Append<uint8_t>(opened);

  std::string const waitOverride;
  out.WriteString(waitOverride);
  out.Append<uint32_t>(0);
}

void BuildGetTicketNoTicket(WorldPacket &out) {
  out.Clear();
  out.SetOpcode(SMSG_GM_TICKET_GET_TICKET);
  out.Append<uint32_t>(kWireStatusDefault);
}

void BuildGmResponseReceived(WorldPacket &out, uint32_t ticketId,
                             std::string const &playerMessage,
                             std::string const &gmResponse) {
  out.Clear();
  out.SetOpcode(SMSG_GMRESPONSE_RECEIVED);
  out.Append<uint32_t>(1u);
  out.Append<uint32_t>(ticketId);
  out.WriteString(playerMessage);

  size_t len = gmResponse.size();
  char const *s = gmResponse.c_str();
  for (int i = 0; i < 4; ++i) {
    if (len > 0) {
      size_t const writeLen = std::min(len, size_t(3999));
      out.Append(reinterpret_cast<uint8_t const *>(s), writeLen);
      len -= writeLen;
      s += writeLen;
    }
    out.Append<uint8_t>(0);
  }
}

ParseCreateResult TryParseTicketCreate(WorldPacket &packet,
                                       ParsedTicketCreate &out) {
  out = {};
  if (packet.Size() - packet.GetReadPos() < sizeof(uint32_t) * 2 + sizeof(float) * 3)
    return ParseCreateResult::TooShort;

  out.mapId = packet.Read<uint32_t>();
  out.x = packet.Read<float>();
  out.y = packet.Read<float>();
  out.z = packet.Read<float>();
  out.message = packet.ReadString();
  if (out.message.size() > kMaxMessageLen)
    return ParseCreateResult::BadPayload;

  if (packet.Size() - packet.GetReadPos() < sizeof(uint32_t) + 1)
    return ParseCreateResult::TooShort;

  out.needResponse = packet.Read<uint32_t>();
  out.needMoreHelp = packet.Read<uint8_t>() != 0;

  if (packet.Size() - packet.GetReadPos() < sizeof(uint32_t))
    return ParseCreateResult::TooShort;
  uint32_t count = packet.Read<uint32_t>();
  for (uint32_t i = 0; i < count; ++i) {
    if (packet.Size() - packet.GetReadPos() < sizeof(uint32_t))
      return ParseCreateResult::TooShort;
    (void)packet.Read<uint32_t>();
  }

  if (packet.Size() - packet.GetReadPos() < sizeof(uint32_t))
    return ParseCreateResult::TooShort;
  uint32_t decompressedSize = packet.Read<uint32_t>();

  if (count && decompressedSize && decompressedSize < 0xFFFF) {
    size_t const pos = packet.GetReadPos();
    size_t const remaining = packet.Size() - pos;
    if (remaining == 0)
      return ParseCreateResult::BadPayload;
    std::vector<uint8_t> dest(decompressedSize);
    uLongf destLen = static_cast<uLongf>(decompressedSize);
    int const zrc = uncompress(dest.data(), &destLen,
                               packet.GetBuffer() + pos,
                               static_cast<uLong>(remaining));
    if (zrc != Z_OK)
      return ParseCreateResult::BadPayload;
    packet.SetReadPos(packet.Size());
  }

  return ParseCreateResult::Ok;
}

} // namespace gm_ticket
} // namespace Firelands
