#pragma once

#include <domain/models/GmTicket.h>
#include <domain/repositories/IGmTicketRepository.h>
#include <application/services/CharacterService.h>
#include <atomic>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace Firelands {

enum class GmTicketCreateResult {
  Success,
  AlreadyExists,
  InvalidCharacter,
  DbError,
};

/// Application rules for GM tickets (ownership, one active ticket, queue touch).
class GmTicketService {
public:
  GmTicketService(std::shared_ptr<IGmTicketRepository> tickets,
                  std::shared_ptr<CharacterService> characters);

  std::optional<GmTicket> GetActiveForCharacter(uint32_t characterGuid);
  std::optional<GmTicket> GetById(uint64_t ticketId) const;

  GmTicketCreateResult CreateTicket(uint32_t accountId, uint32_t characterGuid,
                                   uint32_t mapId, float x, float y, float z,
                                   std::string message, bool needMoreHelp);

  bool UpdateTicketText(uint32_t accountId, uint32_t characterGuid,
                        std::string const &message);

  bool AbandonTicket(uint32_t accountId, uint32_t characterGuid);

  bool ResolveTicket(uint32_t accountId, uint32_t characterGuid);

  bool AssignToStaff(uint64_t ticketId, uint32_t staffAccountId);

  bool StaffReply(uint64_t ticketId, uint32_t staffAccountId,
                  std::string const &response);

  bool StaffClose(uint64_t ticketId, uint32_t staffAccountId);

  std::vector<GmTicket> ListQueue(uint32_t limit) const;
  std::vector<GmTicket> ListAssignedTo(uint32_t staffAccountId,
                                       uint32_t limit) const;

  uint64_t LastQueueChangeUnix() const {
    return _lastQueueChange.load(std::memory_order_relaxed);
  }

  std::optional<uint64_t> OldestActiveUpdatedUnix() const {
    return _tickets->GetOldestActiveUpdatedUnix();
  }

private:
  void TouchQueue();
  bool VerifyCharacterOwnership(uint32_t accountId, uint32_t characterGuid);

  std::shared_ptr<IGmTicketRepository> _tickets;
  std::shared_ptr<CharacterService> _characters;
  std::atomic<uint64_t> _lastQueueChange;
};

} // namespace Firelands
