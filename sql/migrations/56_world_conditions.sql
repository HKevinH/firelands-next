-- World `conditions` table (TrinityCore condition system).
-- Phase rows (SourceType 26) imported in migration 57.
USE `firelands_world`;

CREATE TABLE IF NOT EXISTS `conditions` (
  `SourceTypeOrReferenceId` int NOT NULL DEFAULT '0',
  `SourceGroup` int unsigned NOT NULL DEFAULT '0',
  `SourceEntry` int NOT NULL DEFAULT '0',
  `SourceId` int NOT NULL DEFAULT '0',
  `ElseGroup` int unsigned NOT NULL DEFAULT '0',
  `ConditionTypeOrReference` int NOT NULL DEFAULT '0',
  `ConditionTarget` tinyint unsigned NOT NULL DEFAULT '0',
  `ConditionValue1` int unsigned NOT NULL DEFAULT '0',
  `ConditionValue2` int unsigned NOT NULL DEFAULT '0',
  `ConditionValue3` int unsigned NOT NULL DEFAULT '0',
  `NegativeCondition` tinyint unsigned NOT NULL DEFAULT '0',
  `ErrorType` int unsigned NOT NULL DEFAULT '0',
  `ErrorTextId` int unsigned NOT NULL DEFAULT '0',
  `ScriptName` char(64) NOT NULL DEFAULT '',
  `Comment` varchar(255) DEFAULT NULL,
  PRIMARY KEY (
    `SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`,
    `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`,
    `ConditionValue1`, `ConditionValue2`, `ConditionValue3`
  )
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
