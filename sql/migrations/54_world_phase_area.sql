-- Zone/area → player phase (TrinityCore `phase_area`). Required for correct default phasing
-- (e.g. Gilneas phase 169). Import full ref data when available.
USE `firelands_world`;

CREATE TABLE IF NOT EXISTS `phase_area` (
  `AreaId` int unsigned NOT NULL,
  `PhaseId` int unsigned NOT NULL,
  `Comment` varchar(255) DEFAULT NULL,
  PRIMARY KEY (`AreaId`, `PhaseId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Gilneas / Worgen start (map 654) — Cataclysm default world phase 169
REPLACE INTO `phase_area` (`AreaId`, `PhaseId`, `Comment`) VALUES
(4714, 169, 'Gilneas City'),
(4755, 169, 'Gilneas'),
(4756, 169, 'Gilneas (starter zone id)'),
(4320, 169, 'Gilneas phase area (ref spawns)');
