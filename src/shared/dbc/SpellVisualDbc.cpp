#include <shared/dbc/SpellVisualDbc.h>

#include <shared/dbc/DbcReader.h>
#include <shared/Logger.h>

#include <string_view>
#include <vector>

namespace Firelands {

namespace {

// 4.3.4 client layout: 33 fields, 132-byte records (verified against local `SpellVisual.dbc`).
constexpr std::string_view kSpellVisualFmt =
    "dxxxxxxiixxxxxxxxxxxxxxxxxxxxxxxx";

constexpr uint32_t kFieldId = 0;
constexpr uint32_t kFieldImpactKit = 3;
constexpr uint32_t kFieldTargetImpactKit = 15;

} // namespace

bool SpellVisualDbc::Load(std::string const &path) {
  m_loaded = false;
  m_byId.clear();

  DbcReader reader;
  if (!reader.Load(path)) {
    LOG_WARN("SpellVisual.dbc: failed to load {}", path);
    return false;
  }

  std::vector<uint32_t> const offsets = DbcBuildFieldByteOffsets(kSpellVisualFmt);
  if (!reader.VerifyFormat(kSpellVisualFmt)) {
    LOG_WARN("SpellVisual.dbc: field count {} does not match format {} (path={})",
             reader.GetFieldCount(), kSpellVisualFmt.size(), path);
    return false;
  }

  for (uint32_t rec = 0; rec < reader.GetRecordCount(); ++rec) {
    uint32_t const id = reader.ReadUInt32(rec, kFieldId, offsets);
    if (id == 0u)
      continue;
    SpellVisualRow row{};
    row.id = id;
    row.impactKit = reader.ReadUInt32(rec, kFieldImpactKit, offsets);
    row.targetImpactKit = reader.ReadUInt32(rec, kFieldTargetImpactKit, offsets);
    m_byId.emplace(id, row);
  }

  m_loaded = true;
  LOG_DEBUG("SpellVisual.dbc: {} rows from {}.", m_byId.size(), path);
  return true;
}

std::optional<SpellVisualRow> SpellVisualDbc::GetRow(uint32 visualId) const {
  auto it = m_byId.find(visualId);
  if (it == m_byId.end())
    return std::nullopt;
  return it->second;
}

uint32 SpellVisualDbc::ResolveImpactKitId(uint32 visualId) const {
  auto row = GetRow(visualId);
  if (!row)
    return 0u;
  if (row->targetImpactKit != 0u)
    return row->targetImpactKit;
  return row->impactKit;
}

} // namespace Firelands
