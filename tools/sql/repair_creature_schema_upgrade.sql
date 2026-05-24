-- Repair an existing Firelands DB after the creature schema / import reorder fix.
--
-- Use when world startup logs show:
--   creature_ref_import.sql before firelands_world.sql
--   Table 'firelands_world.creature' doesn't exist
--   gossip/npc_text migrations failing with syntax errors near apostrophes
--
-- Requires a MySQL user that can modify `firelands_world` and `firelands_auth`.
--
-- After running: deploy latest `sql/` (migrations 24–29, renamed creature import),
-- rebuild world (SplitSqlStatements fix), then restart world or auth.
--
-- Docker example (repo root):
--   docker compose exec -T db mysql -uroot -proot < tools/sql/repair_creature_schema_upgrade.sql

USE `firelands_auth`;

DELETE FROM `schema_migrations`
WHERE `migration` IN (
  'creature_ref_import.sql',
  '24_world_creature_tables.sql',
  '25_world_creature_classlevelstats.sql',
  '26_world_instance_tables.sql',
  '29_world_creature_template_modelids.sql',
  'firelands_world_creature_ref_import.sql',
  '31_world_creature_template_gossip_menu_id.sql',
  '34_world_npc_text_data.sql',
  '35_world_gossip_data.sql',
  '38_world_quest_gossip_data.sql',
  '52_world_creature_template_spells.sql',
  '59_world_creature_trigger_phase_fix.sql'
);

-- If creature tables were never created but firelands_world.sql was marked applied
-- under an older bundle, unmark it so the merged bundle can recreate missing DDL.
DELETE FROM `schema_migrations`
WHERE `migration` = 'firelands_world.sql'
  AND NOT EXISTS (
    SELECT 1
    FROM information_schema.TABLES
    WHERE TABLE_SCHEMA = 'firelands_world'
      AND TABLE_NAME = 'creature_template'
  );
