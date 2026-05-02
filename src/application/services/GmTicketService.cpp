#include "GmTicketService.h"
#include <shared/Logger.h>
#include <ctime>

namespace Firelands {

GmTicketService::GmTicketService(std::shared_ptr<IGmTicketRepository> tickets,
                                 std::shared_ptr<CharacterService> characters)
    : _tickets(std::move(tickets)), _characters(std::move(characters)),
      _lastQueueChange(static_cast<uint64_t>(std::time(nullptr))) {}

void GmTicketService::TouchQueue() {
  _lastQueueChange.store(static_cast<uint64_t>(std::time(nullptr)),
                         std::memory_order_relaxed);
}

bool GmTicketService::VerifyCharacterOwnership(uint32_t accountId,
                                             uint32_t characterGuid) {
  auto ch = _characters->GetCharacterByGuid(characterGuid);
  return ch.has_value() && ch->GetAccount() == accountId;
}

std::optional<GmTicket> GmTicketService::GetActiveForCharacter(
    uint32_t characterGuid) {
  return _tickets->FindOpenByCharacterGuid(characterGuid);
}

std::optional<GmTicket> GmTicketService::GetById(uint64_t ticketId) const {
  return _tickets->FindById(ticketId);
}

GmTicketCreateResult GmTicketService::CreateTicket(
    uint32_t accountId, uint32_t characterGuid, uint32_t mapId, float x, float y,
    float z, std::string message, bool needMoreHelp) {
  if (!VerifyCharacterOwnership(accountId, characterGuid))
    return GmTicketCreateResult::InvalidCharacter;
  if (_tickets->FindOpenByCharacterGuid(characterGuid))
    return GmTicketCreateResult::AlreadyExists;

  GmTicket row;
  row.accountId = accountId;
  row.characterGuid = characterGuid;
  row.status = GmTicketStatus::Open;
  row.category = 0;
  row.needMoreHelp = needMoreHelp;
  row.message = std::move(message);
  row.mapId = static_cast<uint16_t>(std::min<uint32_t>(mapId, 65535u));
  row.posX = x;
  row.posY = y;
  row.posZ = z;

  auto const id = _tickets->Insert(row);
  if (!id) {
    LOG_WARN("GmTicketService::CreateTicket insert failed");
    return GmTicketCreateResult::DbError;
  }
  TouchQueue();
  return GmTicketCreateResult::Success;
}

bool GmTicketService::UpdateTicketText(uint32_t accountId, uint32_t characterGuid,
                                       std::string const &message) {
  auto t = _tickets->FindOpenByCharacterGuid(characterGuid);
  if (!t || t->accountId != accountId)
    return false;
  if (!_tickets->UpdateMessage(t->id, accountId, message))
    return false;
  TouchQueue();
  return true;
}

bool GmTicketService::AbandonTicket(uint32_t accountId, uint32_t characterGuid) {
  auto t = _tickets->FindOpenByCharacterGuid(characterGuid);
  if (!t || t->accountId != accountId)
    return false;
  if (!_tickets->CloseByPlayer(t->id, accountId, GmTicketStatus::ClosedAbandoned))
    return false;
  TouchQueue();
  return true;
}

bool GmTicketService::ResolveTicket(uint32_t accountId, uint32_t characterGuid) {
  auto t = _tickets->FindOpenByCharacterGuid(characterGuid);
  if (!t || t->accountId != accountId)
    return false;
  if (t->status != GmTicketStatus::GmAnswered)
    return false;
  if (!_tickets->CloseByPlayer(t->id, accountId, GmTicketStatus::ClosedResolved))
    return false;
  TouchQueue();
  return true;
}

bool GmTicketService::AssignToStaff(uint64_t ticketId, uint32_t staffAccountId) {
  if (!_tickets->TryAssign(ticketId, staffAccountId))
    return false;
  TouchQueue();
  return true;
}

bool GmTicketService::StaffReply(uint64_t ticketId, uint32_t staffAccountId,
                                 std::string const &response) {
  if (!_tickets->SetGmResponseAndStatus(ticketId, staffAccountId, response,
                                        GmTicketStatus::GmAnswered))
    return false;
  TouchQueue();
  return true;
}

bool GmTicketService::StaffClose(uint64_t ticketId, uint32_t staffAccountId) {
  if (!_tickets->CloseByStaff(ticketId, staffAccountId,
                              GmTicketStatus::ClosedStaff))
    return false;
  TouchQueue();
  return true;
}

std::vector<GmTicket> GmTicketService::ListQueue(uint32_t limit) const {
  return _tickets->ListUnassignedOpen(limit);
}

std::vector<GmTicket> GmTicketService::ListAssignedTo(uint32_t staffAccountId,
                                                    uint32_t limit) const {
  return _tickets->ListAssignedToAccount(staffAccountId, limit);
}

} // namespace Firelands
