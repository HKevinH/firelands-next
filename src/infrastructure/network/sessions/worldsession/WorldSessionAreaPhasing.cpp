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

  (void)previousZoneId;

  // Re-bucket zone-dependent chat channels when the area NAME changes. WoW keys
  // these on the SPECIFIC area — cities use the district (e.g. "Valle de Honor"),
  // open zones use the zone name — so we track the raw client area name and swap
  // "<base> - <oldName>" -> "<base> - <newName>".
  if (_playerGuid != 0 && clientAreaHint != 0) {
    if (auto table = runtime().GetAreaTableDbc(); table && table->IsLoaded()) {
      std::string const newName = table->GetName(clientAreaHint);
      if (!newName.empty() && newName != _channelZoneName) {
        LOG_DEBUG("[CHANNEL] area name change '{}' -> '{}' (area={})",
                  _channelZoneName, newName, clientAreaHint);
        if (!_channelZoneName.empty())
          UpdateZoneChannels(_channelZoneName, newName);
        _channelZoneName = newName;
      }
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
