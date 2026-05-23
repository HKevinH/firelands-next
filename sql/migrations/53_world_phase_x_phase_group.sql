-- Phase group membership (Trinity/Cata `phase_x_phase_group`).
-- Required for `creature.PhaseGroup` spawns when `PhaseId` is 0.
-- Import from reference DB2 export or `firelands-cata-ref` when available.

USE `firelands_world`;

CREATE TABLE IF NOT EXISTS `phase_x_phase_group` (
  `ID` int unsigned NOT NULL DEFAULT '0',
  `PhaseID` int NOT NULL DEFAULT '0',
  `PhaseGroupID` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`),
  KEY `idx_phase_group` (`PhaseGroupID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
