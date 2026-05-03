-- Phase E: optional flat mana cost (POWER1) for server-side validation / deduction.
CREATE DATABASE IF NOT EXISTS `firelands_world`;
USE `firelands_world`;

ALTER TABLE `spell_dbc`
  ADD COLUMN `MvpManaCost` int unsigned DEFAULT NULL
  COMMENT 'NULL = none; flat POWER1 cost on cast (SpellDefinition.manaCost)'
  AFTER `MvpDirectHealthDelta`;
