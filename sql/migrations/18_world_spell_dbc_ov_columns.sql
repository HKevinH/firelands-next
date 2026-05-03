-- Optional per-field overrides for spells already in Spell.dbc (NULL = keep DBC).
CREATE DATABASE IF NOT EXISTS `firelands_world`;
USE `firelands_world`;

ALTER TABLE `spell_dbc`
  ADD COLUMN `OvAttributes` int unsigned DEFAULT NULL COMMENT 'Override SpellDefinition.attributes, NULL keeps DBC',
  ADD COLUMN `OvCastingTimeIndex` int unsigned DEFAULT NULL COMMENT 'Override castingTimeIndex, NULL keeps DBC',
  ADD COLUMN `OvDurationIndex` int unsigned DEFAULT NULL COMMENT 'Override durationIndex, NULL keeps DBC',
  ADD COLUMN `OvRangeIndex` int unsigned DEFAULT NULL COMMENT 'Override rangeIndex, NULL keeps DBC',
  ADD COLUMN `OvSchoolMask` int unsigned DEFAULT NULL COMMENT 'Override schoolMask, NULL keeps DBC';
