#pragma once

#include <domain/repositories/IGmTicketRepository.h>
#include <conncpp.hpp>
#include <memory>

namespace Firelands {

class MySqlGmTicketRepository final : public IGmTicketRepository {
public:
  explicit MySqlGmTicketRepository(std::shared_ptr<sql::Connection> connection);

  std::optional<GmTicket> FindOpenByCharacterGuid(uint32_t characterGuid) override;
  std::optional<GmTicket> FindById(uint64_t ticketId) override;
  std::vector<GmTicket> ListUnassignedOpen(uint32_t limit) override;
  std::vector<GmTicket> ListAssignedToAccount(uint32_t staffAccountId,
                                            uint32_t limit) override;
  std::optional<uint64_t> Insert(const GmTicket &ticket) override;
  bool UpdateMessage(uint64_t ticketId, uint32_t playerAccountId,
                     std::string const &message) override;
  bool TryAssign(uint64_t ticketId, uint32_t staffAccountId) override;
  bool SetGmResponseAndStatus(uint64_t ticketId, uint32_t staffAccountId,
                              std::string const &response,
                              GmTicketStatus newStatus) override;
  bool CloseByPlayer(uint64_t ticketId, uint32_t playerAccountId,
                     GmTicketStatus closedStatus) override;
  bool CloseByStaff(uint64_t ticketId, uint32_t staffAccountId,
                    GmTicketStatus closedStatus) override;
  std::optional<uint64_t> GetOldestActiveUpdatedUnix() override;

private:
  std::shared_ptr<sql::Connection> _connection;
  static GmTicket RowToTicket(sql::ResultSet &rs);
  static bool EnsureNeedMoreHelpColumn(std::shared_ptr<sql::Connection> conn);
};

} // namespace Firelands
