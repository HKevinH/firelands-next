#ifndef FIRELANDS_INFRASTRUCTURE_PERSISTENCE_DATABASE_MIGRATION_CATALOG_H
#define FIRELANDS_INFRASTRUCTURE_PERSISTENCE_DATABASE_MIGRATION_CATALOG_H

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace Firelands {

inline constexpr char kAuthDatabase[] = "firelands_auth";
inline constexpr char kCharactersDatabase[] = "firelands_characters";
inline constexpr char kWorldDatabase[] = "firelands_world";

/// Which logical databases a server executable may migrate at startup.
enum class MigrationServerRole { Auth, World };

/// Extracts the JDBC server URI without a `firelands_*` database suffix.
inline std::string ExtractJdbcServerUri(std::string const &jdbcUri) {
  auto const dbPos = jdbcUri.rfind("/firelands_");
  if (dbPos != std::string::npos)
    return jdbcUri.substr(0, dbPos);
  return jdbcUri;
}

/// Resolves the JDBC URI for a known Firelands database name.
inline std::string JdbcUriForDatabase(std::string const &serverUri,
                                      std::string const &database) {
  return serverUri + "/" + database;
}

/// Classifies a migration/init SQL file to its target database (merge_migrations.py parity).
inline std::optional<std::string>
DetectMigrationTargetDatabase(std::string const &filename,
                              std::string const &content) {
  auto const findQuotedDb = [&](char const *pattern, size_t patternLen)
      -> std::optional<std::string> {
    auto const pos = content.find(pattern, 0);
    if (pos == std::string::npos)
      return std::nullopt;
    auto const openingTick = content.find('`', pos);
    if (openingTick == std::string::npos)
      return std::nullopt;
    auto const closingTick = content.find('`', openingTick + 1);
    if (closingTick == std::string::npos)
      return std::nullopt;
    return content.substr(openingTick + 1, closingTick - openingTick - 1);
  };

  if (auto db = findQuotedDb("USE `", 5))
    return db;
  if (auto db = findQuotedDb("CREATE DATABASE IF NOT EXISTS `", 31))
    return db;

  for (char const *known :
       {kAuthDatabase, kCharactersDatabase, kWorldDatabase}) {
    if (content.find(std::string{"`"} + known + "`.") != std::string::npos)
      return std::string{known};
  }

  std::string lower = filename;
  std::transform(lower.begin(), lower.end(), lower.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

  if (lower.find("auth") != std::string::npos &&
      lower.find("character") == std::string::npos)
    return std::string{kAuthDatabase};
  if (lower.find("item_instance") != std::string::npos ||
      lower.find("characters") != std::string::npos)
    return std::string{kCharactersDatabase};
  if (lower.find("world") != std::string::npos)
    return std::string{kWorldDatabase};

  return std::nullopt;
}

inline std::vector<std::string>
TargetDatabasesForRole(MigrationServerRole role) {
  if (role == MigrationServerRole::Auth)
    return {kAuthDatabase};
  // World uses auth for accounts, RBAC, realm gates — apply auth migrations here too.
  return {kAuthDatabase, kCharactersDatabase, kWorldDatabase};
}

/// Init SQL paths for a database, in execution order.
inline std::vector<std::filesystem::path>
InitScriptPathsForDatabase(std::filesystem::path const &sqlDir,
                           std::string const &database) {
  std::filesystem::path const initDir = sqlDir / "init";
  std::vector<std::filesystem::path> paths;
  if (database == kAuthDatabase)
    paths.push_back(initDir / "auth_schema.sql");
  else if (database == kCharactersDatabase)
    paths.push_back(initDir / "characters_schema.sql");
  else if (database == kWorldDatabase) {
    paths.push_back(initDir / "world_schema.sql");
    paths.push_back(initDir / "world_schema_quest.sql");
    paths.push_back(initDir / "world_schema_creature.sql");
    paths.push_back(initDir / "world_schema_gossip.sql");
    paths.push_back(initDir / "world_schema_spell.sql");
    paths.push_back(initDir / "world_schema_player.sql");
    paths.push_back(initDir / "world_schema_gameobject.sql");
    paths.push_back(initDir / "world_seed_quest_data.sql");
    paths.push_back(initDir / "world_seed_quest_template_addon_data.sql");
    paths.push_back(initDir / "world_seed_creature_data.sql");
    paths.push_back(initDir / "world_seed_creature_spawn_data.sql");
    paths.push_back(initDir / "world_seed_creature_addon_data.sql");
    paths.push_back(initDir / "world_seed_creature_model_info_data.sql");
    paths.push_back(initDir / "world_seed_creature_queststarter_data.sql");
    paths.push_back(initDir / "world_seed_creature_questender_data.sql");
    paths.push_back(initDir / "world_seed_creature_template_addon_data.sql");
    paths.push_back(initDir / "world_seed_creature_equip_template_data.sql");
    paths.push_back(initDir / "world_seed_creature_text_data.sql");
    paths.push_back(initDir / "world_seed_npc_vendor_data.sql");
    paths.push_back(initDir / "world_seed_gameobject_data.sql");
    paths.push_back(initDir / "world_seed_gameobject_template_addon_data.sql");
    paths.push_back(initDir / "world_seed_gameobject_spawn_data.sql");
    paths.push_back(initDir / "world_seed_gameobject_addon_data.sql");
    paths.push_back(initDir / "world_seed_gameobject_queststarter_data.sql");
    paths.push_back(initDir / "world_seed_gameobject_questender_data.sql");
    paths.push_back(initDir / "world_seed_gossip_data.sql");
    paths.push_back(initDir / "world_seed_gossip_action_data.sql");
    paths.push_back(initDir / "world_seed_gossip_box_data.sql");
    paths.push_back(initDir / "world_seed_gossip_menu_data.sql");
    paths.push_back(initDir / "world_seed_npc_text_data.sql");
    paths.push_back(initDir / "world_seed_spell_dbc_data.sql");
    paths.push_back(initDir / "world_seed_spelldbc_data.sql");
    paths.push_back(initDir / "world_seed_spell_area_data.sql");
    paths.push_back(initDir / "world_seed_spell_group_data.sql");
    paths.push_back(initDir / "world_seed_spell_group_stack_rules_data.sql");
    paths.push_back(initDir / "world_seed_spell_learn_spell_data.sql");
    paths.push_back(initDir / "world_seed_spell_linked_spell_data.sql");
    paths.push_back(initDir / "world_seed_spell_proc_data.sql");
    paths.push_back(initDir / "world_seed_spell_rank_data.sql");
    paths.push_back(initDir / "world_seed_spell_required_data.sql");
    paths.push_back(initDir / "world_seed_spell_target_position_data.sql");
    paths.push_back(initDir / "world_seed_spelldifficulty_dbc_data.sql");
    paths.push_back(initDir / "world_seed_playercreateinfo_data.sql");
    paths.push_back(initDir / "world_seed_playercreateinfo_action_data.sql");
    paths.push_back(initDir / "world_seed_playercreateinfo_cast_spell_data.sql");
    paths.push_back(initDir / "world_seed_playercreateinfo_item_data.sql");
    paths.push_back(initDir / "world_seed_playercreateinfo_skills_data.sql");
    paths.push_back(initDir / "world_seed_playercreateinfo_spell_custom_data.sql");
    paths.push_back(initDir / "world_seed_player_levelstats_data.sql");
    paths.push_back(initDir / "world_seed_player_xp_for_level_data.sql");
    paths.push_back(initDir / "world_seed_player_racestats_data.sql");
    paths.push_back(initDir / "world_seed.sql");
  }
  std::vector<std::filesystem::path> existing;
  for (auto const &p : paths) {
    if (std::filesystem::is_regular_file(p))
      existing.push_back(p);
  }
  return existing;
}

inline std::pair<int, std::string> MigrationNumericSortKey(std::string const &path) {
  std::string const name = std::filesystem::path(path).filename().string();
  auto const underscore = name.find('_');
  if (underscore != std::string::npos) {
    try {
      return {std::stoi(name.substr(0, underscore)), name};
    } catch (...) {
    }
  }
  return {999999, name};
}

/// Sorted `sql/migrations/*.sql` paths (numeric prefix order).
inline std::vector<std::filesystem::path>
CollectMigrationSqlPaths(std::filesystem::path const &sqlDir) {
  std::filesystem::path const migrationsDir = sqlDir / "migrations";
  std::vector<std::filesystem::path> paths;
  if (!std::filesystem::is_directory(migrationsDir))
    return paths;
  for (auto const &entry : std::filesystem::directory_iterator(migrationsDir)) {
    if (entry.is_regular_file() && entry.path().extension() == ".sql")
      paths.push_back(entry.path());
  }
  std::sort(paths.begin(), paths.end(), [](auto const &a, auto const &b) {
    return MigrationNumericSortKey(a.string()) < MigrationNumericSortKey(b.string());
  });
  return paths;
}

inline std::string ReadTextFile(std::filesystem::path const &path) {
  std::ifstream file(path);
  if (!file.is_open())
    return {};
  return std::string{std::istreambuf_iterator<char>(file),
                     std::istreambuf_iterator<char>()};
}

/// Migration files under `sql/migrations/` that apply to `database`.
inline std::vector<std::filesystem::path>
MigrationPathsForDatabase(std::filesystem::path const &sqlDir,
                          std::string const &database) {
  std::vector<std::filesystem::path> result;
  for (auto const &path : CollectMigrationSqlPaths(sqlDir)) {
    std::string const content = ReadTextFile(path);
    auto const target = DetectMigrationTargetDatabase(path.filename().string(), content);
    if (target && *target == database)
      result.push_back(path);
  }
  return result;
}

} // namespace Firelands

#endif // FIRELANDS_INFRASTRUCTURE_PERSISTENCE_DATABASE_MIGRATION_CATALOG_H
