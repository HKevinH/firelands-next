#pragma once

#include <domain/models/GossipMenu.h>
#include <cstdint>
#include <string>
#include <vector>

namespace Firelands {

/// Quest line icon in `SMSG_GOSSIP_MESSAGE` (`ClientGossipText::QuestType` / mangos
/// `QUEST_ICON_*`). Not the same as overhead `QuestGiverDialogStatus` flags.
enum class QuestGossipIcon : uint8_t {
  None = 0,
  Unavailable = 1,
  LowLevel = 2,
  Available = 3,
  Complete = 4,
};

/// Minimal quest fields required to render a gossip quest line.
struct QuestGossipSummary {
  uint32_t questId = 0;
  std::string title;
  int32_t questLevel = 0;
  uint32_t flags = 0;
  bool isAutoComplete = false;
};

/// Trinity `QUEST_FLAGS_AUTOCOMPLETE` (0x00010000).
inline constexpr uint32_t kQuestFlagAutoComplete = 0x00010000u;

inline bool QuestHasAutoCompleteFlag(uint32_t flags) noexcept {
  return (flags & kQuestFlagAutoComplete) != 0;
}

inline GossipQuestItem ToGossipQuestItem(QuestGossipSummary const &summary,
                                         QuestGossipIcon icon) {
  GossipQuestItem item;
  item.questId = summary.questId;
  item.questIcon = static_cast<uint8_t>(icon);
  item.questLevel = summary.questLevel;
  item.questFlags = summary.flags;
  item.isAutoComplete = summary.isAutoComplete;
  item.questTitle = summary.title;
  return item;
}

} // namespace Firelands
