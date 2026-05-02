#include <infrastructure/network/sessions/WorldSession.h>
#include <application/services/GmTicketService.h>
#include <infrastructure/network/sessions/worldsession/GmTicketPackets.h>
#include <shared/network/WorldOpcodes.h>
#include <shared/network/WorldPacket.h>
#include <shared/Logger.h>

namespace Firelands {

void WorldSession::SendGmResponseReceived(
    uint32_t ticketId, std::string const &playerMessage,
    std::string const &gmResponse) {
  WorldPacket pkt(0, 256);
  gm_ticket::BuildGmResponseReceived(pkt, ticketId, playerMessage, gmResponse);
  SendPacket(pkt);
}

void WorldSession::HandleGmTicketCreate(WorldPacket &packet) {
  if (!_gmTicketService || _playerGuid == 0)
    return;
  gm_ticket::ParsedTicketCreate parsed;
  auto const pr = gm_ticket::TryParseTicketCreate(packet, parsed);
  if (pr != gm_ticket::ParseCreateResult::Ok) {
    WorldPacket res(SMSG_GM_TICKET_CREATE, 8);
    res.Append<uint32_t>(gm_ticket::kWireResponseCreateError);
    SendPacket(res);
    return;
  }

  auto const result = _gmTicketService->CreateTicket(
      _accountId, static_cast<uint32_t>(_playerGuid), parsed.mapId, parsed.x,
      parsed.y, parsed.z, parsed.message, parsed.needMoreHelp);

  WorldPacket res(SMSG_GM_TICKET_CREATE, 8);
  switch (result) {
  case GmTicketCreateResult::Success:
    res.Append<uint32_t>(gm_ticket::kWireResponseCreateSuccess);
    break;
  case GmTicketCreateResult::AlreadyExists:
    res.Append<uint32_t>(gm_ticket::kWireResponseAlreadyExist);
    break;
  default:
    res.Append<uint32_t>(gm_ticket::kWireResponseCreateError);
    break;
  }
  SendPacket(res);
}

void WorldSession::HandleGmTicketUpdateText(WorldPacket &packet) {
  if (!_gmTicketService || _playerGuid == 0)
    return;
  std::string const msg = packet.ReadString();
  bool const ok = _gmTicketService->UpdateTicketText(
      _accountId, static_cast<uint32_t>(_playerGuid), msg);
  WorldPacket res(SMSG_GM_TICKET_UPDATE_TEXT, 8);
  res.Append<uint32_t>(ok ? gm_ticket::kWireResponseUpdateSuccess
                           : gm_ticket::kWireResponseUpdateError);
  SendPacket(res);
}

void WorldSession::HandleGmTicketDelete(WorldPacket & /*packet*/) {
  if (!_gmTicketService || _playerGuid == 0)
    return;
  if (!_gmTicketService->AbandonTicket(_accountId,
                                       static_cast<uint32_t>(_playerGuid)))
    return;
  WorldPacket del(SMSG_GM_TICKET_DELETE_TICKET, 8);
  del.Append<uint32_t>(gm_ticket::kWireResponseTicketDeleted);
  SendPacket(del);
  WorldPacket none(0, 64);
  gm_ticket::BuildGetTicketNoTicket(none);
  SendPacket(none);
}

void WorldSession::HandleGmTicketGetTicket(WorldPacket & /*packet*/) {
  if (!_gmTicketService || _playerGuid == 0)
    return;
  SendQueryTimeResponse();
  auto const t =
      _gmTicketService->GetActiveForCharacter(static_cast<uint32_t>(_playerGuid));
  if (!t) {
    WorldPacket pkt(0, 32);
    gm_ticket::BuildGetTicketNoTicket(pkt);
    SendPacket(pkt);
    return;
  }
  if (t->status == GmTicketStatus::GmAnswered && !t->gmResponse.empty()) {
    SendGmResponseReceived(static_cast<uint32_t>(t->id), t->message, t->gmResponse);
    return;
  }
  auto const oldest = _gmTicketService->OldestActiveUpdatedUnix();
  WorldPacket pkt(0, 512);
  gm_ticket::BuildGetTicketHasText(pkt, *t, oldest,
                                  _gmTicketService->LastQueueChangeUnix());
  SendPacket(pkt);
}

void WorldSession::HandleGmTicketSystemStatus(WorldPacket & /*packet*/) {
  WorldPacket res(SMSG_GM_TICKET_GET_SYSTEM_STATUS, 8);
  res.Append<uint32_t>(gm_ticket::kQueueStatusEnabled);
  SendPacket(res);
}

void WorldSession::HandleGmTicketResponseResolve(WorldPacket & /*packet*/) {
  if (!_gmTicketService || _playerGuid == 0)
    return;
  if (!_gmTicketService->ResolveTicket(_accountId,
                                       static_cast<uint32_t>(_playerGuid)))
    return;
  WorldPacket st(SMSG_GMRESPONSE_STATUS_UPDATE, 8);
  st.Append<uint8_t>(0);
  SendPacket(st);
  WorldPacket del(SMSG_GM_TICKET_DELETE_TICKET, 8);
  del.Append<uint32_t>(gm_ticket::kWireResponseTicketDeleted);
  SendPacket(del);
  WorldPacket none(0, 32);
  gm_ticket::BuildGetTicketNoTicket(none);
  SendPacket(none);
}

} // namespace Firelands
