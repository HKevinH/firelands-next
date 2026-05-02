#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace Firelands {

/// Wire-aligned-ish statuses for DB + domain; map to client expectations in handlers.
enum class GmTicketStatus : uint8_t {
  Open = 0,
  Assigned = 1,
  GmAnswered = 2,
  ClosedResolved = 3,
  ClosedAbandoned = 4,
  ClosedStaff = 5,
};

struct GmTicket {
  uint64_t id = 0;
  uint32_t accountId = 0;
  uint32_t characterGuid = 0;
  GmTicketStatus status = GmTicketStatus::Open;
  uint8_t category = 0;
  bool needMoreHelp = false;
  std::string message;
  std::string gmResponse;
  uint16_t mapId = 0;
  float posX = 0.f;
  float posY = 0.f;
  float posZ = 0.f;
  std::optional<uint32_t> assignedAccountId;
  /// Unix ms optional; repositories may use DB timestamps instead.
  uint64_t createdAtUnixMs = 0;
  uint64_t updatedAtUnixMs = 0;
};

} // namespace Firelands
