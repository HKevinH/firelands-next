#include <application/logic/GossipLogic.h>
#include <application/services/WorldService.h>
#include <domain/repositories/IGossipRepository.h>
#include <domain/repositories/INpcTemplateSearchRepository.h>
#include <domain/world/Creature.h>
#include <domain/world/Map.h>
#include <infrastructure/network/sessions/WorldSession.h>
#include <shared/game/WowGuid.h>

namespace Firelands {

std::optional<uint32_t> WorldSession::TryResolveCreatureTemplateEntry(
    uint64_t npcGuid) const {
  if (npcGuid == 0)
    return std::nullopt;

  if (_mapId != 0) {
    if (auto map = WorldService::Instance().GetMap(_mapId)) {
      if (auto creature = map->TryGetCreature(npcGuid))
        return creature->GetEntry();
    }
  }

  uint32_t const fromGuid = ExtractCreatureEntryFromUnitObjectGuid(npcGuid);
  return fromGuid != 0 ? std::optional<uint32_t>{fromGuid} : std::nullopt;
}

bool WorldSession::TrySendDatabaseGossipMenu(uint64_t npcGuid,
                                             uint32_t templateEntry) {
  if (!_gossipRepo || templateEntry == 0)
    return false;

  uint32_t menuId = kDefaultNpcGossipMenuId;
  uint64_t npcFlags = 0;

  if (_npcTemplateSearch) {
    if (auto const row = _npcTemplateSearch->TryGetByEntry(templateEntry)) {
      menuId = ResolveGossipMenuIdForTemplate(row->gossipMenuId);
      npcFlags = row->npcFlags;
    }
  }

  auto options = _gossipRepo->GetMenuOptions(menuId);
  options = FilterGossipOptionsByNpcFlags(std::move(options), npcFlags);

  auto const textId = _gossipRepo->GetMenuTextId(menuId);
  if (menuId == kDefaultNpcGossipMenuId && options.empty() && !textId)
    return false;

  SendGossipMessage(npcGuid, menuId, textId.value_or(0), options);
  return true;
}

} // namespace Firelands
