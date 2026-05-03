-- Remove Phase D/E MVP columns from spell_dbc; health deltas come from SpellEffect.dbc;
-- mana cost from SpellPower.dbc only.

USE `firelands_world`;

SET @exist :=
  (SELECT COUNT(*) FROM INFORMATION_SCHEMA.COLUMNS
   WHERE TABLE_SCHEMA = DATABASE()
     AND TABLE_NAME = 'spell_dbc'
     AND COLUMN_NAME = 'MvpManaCost');

SET @sqlstmt := IF(@exist > 0,
  'ALTER TABLE `spell_dbc` DROP COLUMN `MvpManaCost`',
  'SELECT 1');

PREPARE stmt FROM @sqlstmt;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @exist :=
  (SELECT COUNT(*) FROM INFORMATION_SCHEMA.COLUMNS
   WHERE TABLE_SCHEMA = DATABASE()
     AND TABLE_NAME = 'spell_dbc'
     AND COLUMN_NAME = 'MvpDirectHealthDelta');

SET @sqlstmt := IF(@exist > 0,
  'ALTER TABLE `spell_dbc` DROP COLUMN `MvpDirectHealthDelta`',
  'SELECT 1');

PREPARE stmt FROM @sqlstmt;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
