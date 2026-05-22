#pragma once

#include <shared/Common.h>

#include <optional>
#include <string>
#include <unordered_map>

namespace Firelands {

/// Row from client `SpellVisual.dbc` (Cataclysm 4.3.4 / build 15595).
struct SpellVisualRow {
  uint32 id = 0;
  uint32 impactKit = 0;
  uint32 targetImpactKit = 0;
};

/// Loads `SpellVisual.dbc` for server-driven impact VFX (`SMSG_PLAY_SPELL_VISUAL_KIT`).
class SpellVisualDbc {
public:
  bool Load(std::string const &path);

  bool IsLoaded() const { return m_loaded; }

  std::optional<SpellVisualRow> GetRow(uint32 visualId) const;

  /// `TargetImpactKit` when set, else `ImpactKit` (reference `SpellVisual` columns 15 / 3).
  uint32 ResolveImpactKitId(uint32 visualId) const;

private:
  bool m_loaded = false;
  std::unordered_map<uint32, SpellVisualRow> m_byId;
};

} // namespace Firelands
