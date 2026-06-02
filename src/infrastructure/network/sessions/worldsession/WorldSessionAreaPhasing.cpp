#include <infrastructure/network/sessions/WorldSession.h>
#include <shared/dbc/AreaTableDbc.h>
#include <shared/network/UpdateData.h>
#include <shared/network/UpdateFields.h>
#include <shared/network/WorldOpcodes.h>
#include <shared/network/WorldPacket.h>
#include <shared/Logger.h>
#include <map>

namespace Firelands {

uint32_t WorldSession::ResolveSessionAreaId(uint32_t clientAreaHint) const {
  if (_mapId == 0 || clientAreaHint == 0)
    return clientAreaHint;
  if (auto table = runtime().GetAreaTableDbc()) {
    if (table->IsLoaded())
      return table->ResolveAreaForPhasing(_mapId, clientAreaHint);
  }
  return clientAreaHint;
}

void WorldSession::SetSessionAreaId(uint32_t clientAreaHint) {
  uint32_t const resolved = ResolveSessionAreaId(clientAreaHint);
  uint32_t const previousZoneId = _zoneId;
  if (resolved != 0 && resolved != _zoneId)
    _zoneId = resolved;

  // Re-bucket zone-dependent chat channels (General/LocalDefense) when the
  // zone-level name changes. We resolve each area up to its top-level zone so
  // moving between subzones of the same zone does not churn channels.
  if (_playerGuid != 0 && resolved != 0 && resolved != previousZoneId) {
    if (auto table = runtime().GetAreaTableDbc(); table && table->IsLoaded()) {
      auto zoneTop = [&](uint32_t areaId) {
        uint32_t cur = areaId;
        for (int i = 0; i < 8 && cur != 0; ++i) {
          uint32_t const parent = table->GetParentAreaId(cur);
          if (parent == 0)
            break;
          cur = parent;
        }
        return cur;
      };
      std::string const oldZone = table->GetName(zoneTop(previousZoneId));
      std::string const newZone = table->GetName(zoneTop(resolved));
      LOG_DEBUG("[CHANNEL] zone change prevArea={} newArea={} oldZone='{}' "
                "newZone='{}'",
                previousZoneId, resolved, oldZone, newZone);
      if (!oldZone.empty() && !newZone.empty() && oldZone != newZone)
        UpdateZoneChannels(oldZone, newZone);
    }
  }

  // Reveal the map for the SPECIFIC area the player is in. Use the raw client area
  // (not the phase-resolved one, which can collapse sub-areas to the zone), so each
  // sub-area of a big zone reveals its own portion as you walk into it.
  if (_playerGuid != 0 && clientAreaHint != 0)
    DiscoverArea(clientAreaHint);

  if (resolved == _areaId)
    return;
  _areaId = resolved;
  if (_playerGuid != 0)
    RefreshPlayerPhaseVisibilityFromAuras();
}

void WorldSession::DiscoverArea(uint32_t areaId) {
  if (_playerGuid == 0 || areaId == 0)
    return;
  auto table = runtime().GetAreaTableDbc();
  if (!table || !table->IsLoaded())
    return;
  uint32_t const areaBit = table->GetAreaBit(areaId);
  if (areaBit == 0)
    return;  // area has no exploration bit

  uint16_t const fieldIndex =
      static_cast<uint16_t>(PLAYER_EXPLORED_ZONES_1 + (areaBit / 32u));
  uint32_t const bitMask = 1u << (areaBit % 32u);
  uint32_t &block = _exploredZoneBlocks[fieldIndex];
  if (block & bitMask)
    return;  // already discovered this session
  block |= bitMask;

  // Push the explored-zones field so the client reveals this area's map.
  std::map<uint16, uint32> fields;
  fields[fieldIndex] = block;
  UpdateData update(_mapId);
  update.AddValuesUpdate(_playerGuid, fields);
  WorldPacket pkt(static_cast<uint32>(SMSG_UPDATE_OBJECT));
  update.Build(pkt);
  SendPacket(pkt);

  std::string const name = table->GetName(areaId);
  if (!name.empty())
    SendNotification("Descubierto: " + name);
  LOG_DEBUG("[EXPLORE] guid={} area={} areaBit={} name='{}'", _playerGuid, areaId,
            areaBit, name);
}

} // namespace Firelands
