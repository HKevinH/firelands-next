#pragma once

#include <domain/models/GmTicket.h>
#include <cstdint>
#include <optional>
#include <vector>

namespace Firelands {

/// Persistence for GM help tickets (`firelands_characters.gm_ticket`).
/// All mutators that take `accountId` must enforce ownership or staff rules in the service layer.
class IGmTicketRepository {
public:
  virtual ~IGmTicketRepository() = default;

  /// Active ticket for this character (open, assigned, or waiting on player after GM reply).
  virtual std::optional<GmTicket> FindOpenByCharacterGuid(uint32_t characterGuid) = 0;

  virtual std::optional<GmTicket> FindById(uint64_t ticketId) = 0;

  /// Open tickets with no assignee (FIFO queue).
  virtual std::vector<GmTicket> ListUnassignedOpen(uint32_t limit) = 0;

  /// Tickets assigned to a staff account (typically GM client session).
  virtual std::vector<GmTicket> ListAssignedToAccount(uint32_t staffAccountId,
                                                      uint32_t limit) = 0;

  virtual std::optional<uint64_t> Insert(const GmTicket &ticket) = 0;

  virtual bool UpdateMessage(uint64_t ticketId, uint32_t playerAccountId,
                             std::string const &message) = 0;

  /// Sets `assigned_account_id` and status Assigned only if currently unassigned and Open.
  virtual bool TryAssign(uint64_t ticketId, uint32_t staffAccountId) = 0;

  virtual bool SetGmResponseAndStatus(uint64_t ticketId, uint32_t staffAccountId,
                                      std::string const &response,
                                      GmTicketStatus newStatus) = 0;

  virtual bool CloseByPlayer(uint64_t ticketId, uint32_t playerAccountId,
                             GmTicketStatus closedStatus) = 0;

  virtual bool CloseByStaff(uint64_t ticketId, uint32_t staffAccountId,
                            GmTicketStatus closedStatus) = 0;

  /// Minimum `updated_at` among tickets still in player-visible workflow (open/assigned/answered).
  virtual std::optional<uint64_t> GetOldestActiveUpdatedUnix() = 0;
};

} // namespace Firelands
