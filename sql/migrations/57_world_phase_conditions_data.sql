-- Phase area conditions (quest/aura gates for zone phasing).
-- Reference: firelands-cata-ref `conditions` (SourceType 26).
-- JDBC-safe: DELETE type-26 rows + batched REPLACE.
-- Regenerate: python3 tools/sql/import_ref_phase_conditions.py
-- Requires migration 56 (conditions DDL).

USE `firelands_world`;

DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 26;

REPLACE INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `ErrorType`, `ErrorTextId`, `ScriptName`, `Comment`) VALUES
(26,105,4714,0,0,8,0,14222,0,0,0,0,0,'','Gilneas - Phase 105 - active when rewarded quest 14222'),
(26,169,4765,0,0,9,0,14153,0,0,0,0,0,'','Set Phase 169 in area 4765; Kezan, Quest Life of the party (14153)'),
(26,169,5140,0,1,8,0,28598,0,0,0,0,0,'','Set phase 169 in area 5140 if quest 28598 rewarded'),
(26,169,5140,0,2,28,0,28598,0,0,0,0,0,'','Set phase 169 in area 5140 if quest 28598 complete'),
(26,169,5424,0,1,8,0,28598,0,0,0,0,0,'','Set phase 169 in area 5140 if quest 28598 rewarded'),
(26,169,5424,0,2,28,0,28598,0,0,0,0,0,'','Set phase 169 in area 5140 if quest 28598 complete'),
(26,181,4806,0,0,9,0,14400,0,0,0,0,0,'','Phase only if quest 14400 is taken'),
(26,226,108,0,0,8,0,26322,0,0,0,0,0,'','Phase 226: active when rewarded quest 26322'),
(26,312,5602,0,0,1,0,88111,0,0,0,0,0,'','Set Phase to 312 for area 5602 if player has aura Phase Player.'),
(26,313,5602,0,1,1,0,88111,0,0,1,0,0,'','Set Phase to 313 for area 5602 if player does not hav aura Phase Player.'),
(26,313,5602,0,1,8,0,27950,0,0,1,0,0,'','Set Phase to 313 for area 5602 if quest Gobbles! has not been rewarded.'),
(26,313,5700,0,1,1,0,88111,0,0,1,0,0,'','Set Phase to 313 for area 5700 if player does not hav aura Phase Player.'),
(26,313,5700,0,1,8,0,27950,0,0,1,0,0,'','Set Phase to 313 for area 5700 if quest Gobbles! has not been rewarded.'),
(26,315,5602,0,1,8,0,27950,0,0,0,0,0,'','Set Phase to 315 for area 5602 if quest Gobbles! has been rewarded.'),
(26,315,5602,0,1,8,0,28002,0,0,1,0,0,'','Set Phase to 315 for area 5602 if quest Crisis Management has not been rewarded.'),
(26,315,5700,0,1,8,0,27950,0,0,0,0,0,'','Set Phase to 315 for area 5700 if quest Gobbles! has been rewarded.'),
(26,315,5700,0,1,8,0,28002,0,0,1,0,0,'','Set Phase to 315 for area 5700 if quest Crisis Management has not been rewarded.'),
(26,324,5602,0,0,8,0,28002,0,0,0,0,0,'','Set Phase to 324 for area 5602 if quest Crisis Management has been rewarded.'),
(26,324,5700,0,0,8,0,28002,0,0,0,0,0,'','Set Phase to 324 for area 5700 if quest Crisis Management has been rewarded.'),
(26,361,5140,0,1,8,0,28598,0,0,1,0,0,'','Set phase 361 in area 5140 if quest 28598 not rewarded'),
(26,361,5140,0,2,28,0,28598,0,0,1,0,0,'','Set phase 361 in area 5140 if quest 28598 not complete'),
(26,361,5424,0,1,8,0,28598,0,0,1,0,0,'','Set phase 361 in area 5140 if quest 28598 not rewarded'),
(26,361,5424,0,2,28,0,28598,0,0,1,0,0,'','Set phase 361 in area 5140 if quest 28598 not complete'),
(26,379,4765,0,0,9,0,14153,0,0,0,0,0,'','Set Phase 379 in area 4765; Kezan, Quest Life of the party (14153)');
