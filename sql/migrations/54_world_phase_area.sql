-- Zone/area → player phase (TrinityCore `phase_area`). Seed data in migration 55
-- (`python3 tools/sql/import_ref_phase_data.py`).
USE `firelands_world`;

CREATE TABLE IF NOT EXISTS `phase_area` (
  `AreaId` int unsigned NOT NULL,
  `PhaseId` int unsigned NOT NULL,
  `Comment` varchar(255) DEFAULT NULL,
  PRIMARY KEY (`AreaId`, `PhaseId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Data: migration 55 (`python3 tools/sql/import_ref_phase_data.py`).
