#pragma once

#include <domain/models/GossipMenu.h>
#include <shared/network/WorldOpcodes.h>
#include <shared/network/WorldPacket.h>
#include <cstdint>
#include <string>
#include <vector>

namespace Firelands {

/// Cataclysm 4.3.4 gossip wire helpers.
/// Reference: `PlayerMenu::SendGossipMenu` / `PlayerMenu::SendCloseGossip`.
namespace gossip {

/// Build `SMSG_GOSSIP_MESSAGE` — gossip options only (no quest items).
/// Cataclysm 4.3.4 layout (TCPP `WorldPackets::NPC::GossipMessage` / `SGossipOptions`).
inline WorldPacket BuildGossipMessage(uint64_t npcGuid,
                                      uint32_t menuId, uint32_t textId,
                                      std::vector<GossipMenuItem> const &items) {
  WorldPacket out(SMSG_GOSSIP_MESSAGE, 100);
  // TCPP 4.3.4: `operator<<(ObjectGuid)` writes a full uint64, not a packed GUID.
  out.Append<uint64_t>(npcGuid);
  out.Append<int32_t>(static_cast<int32_t>(menuId));
  out.Append<int32_t>(static_cast<int32_t>(textId));
  out.Append<uint32_t>(static_cast<uint32_t>(items.size()));

  for (auto const &item : items) {
    out.Append<int32_t>(static_cast<int32_t>(item.optionIndex));
    out.Append<uint8_t>(static_cast<uint8_t>(item.icon));
    out.Append<int8_t>(item.isCoded ? 1 : 0);
    out.Append<int32_t>(static_cast<int32_t>(item.boxMoney));
    out.WriteString(item.optionText);
    out.WriteString(item.boxMessage);
  }

  // Quest gossip lines (0 until quest menu is implemented).
  out.Append<uint32_t>(0);
  return out;
}

/// Build `SMSG_GOSSIP_COMPLETE` — closes the gossip window.
inline WorldPacket BuildGossipComplete() {
  WorldPacket out(SMSG_GOSSIP_COMPLETE, 0);
  return out;
}

/// Build `SMSG_GOSSIP_MESSAGE` with a single text-only greeting.
inline WorldPacket BuildGossipMessageSimple(uint64_t npcGuid,
                                            uint32_t menuId, uint32_t textId) {
  std::vector<GossipMenuItem> empty;
  return BuildGossipMessage(npcGuid, menuId, textId, empty);
}

} // namespace gossip

} // namespace Firelands
