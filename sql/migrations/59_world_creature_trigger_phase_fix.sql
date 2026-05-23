-- Fix trigger creature spawns corrupted by off-by-one PhaseId import (PhaseId=169, PhaseGroup=0).
-- Reference pattern for CREATURE_FLAG_EXTRA_TRIGGER spawns: PhaseId=1, PhaseGroup=169.
USE `firelands_world`;

UPDATE `creature` AS c
INNER JOIN `creature_template` AS ct ON ct.`entry` = c.`id`
SET c.`PhaseId` = 1,
    c.`PhaseGroup` = 169
WHERE (ct.`flags_extra` & 128) <> 0
  AND c.`phaseUseFlags` = 0
  AND c.`PhaseId` = 169
  AND c.`PhaseGroup` = 0;
