-- Warpweavers in the Stormwind and Orgrimmar banks, on Shattrath's Terrace of Light and in Dalaran's Runeweaver Square.
DELETE FROM `creature` WHERE `guid` BETWEEN 9102001 AND 9102004;
INSERT INTO `creature` (`guid`, `id`, `map`, `zoneId`, `areaId`, `spawnMask`, `phaseMask`, `position_x`, `position_y`,
    `position_z`, `orientation`, `spawntimesecs`, `MovementType`, `Comment`)
VALUES
(9102001, 190010, 0, 1519, 0, 1, 1, -8920.1, 614.3, 99.6, 2.05, 300, 0, 'mod-transmog: Stormwind bank'),
(9102002, 190010, 1, 1637, 0, 1, 1, 1622.8, -4382.9, 12.1, 2.02, 300, 0, 'mod-transmog: Orgrimmar bank'),
(9102003, 190010, 571, 4395, 0, 1, 1, 5812.0, 631.0, 647.65, 2.16, 300, 0, 'mod-transmog: Dalaran, Runeweaver Square'),
(9102004, 190010, 530, 3703, 0, 1, 1, -1845.1, 5406.5, -12.34, 2.24, 300, 0, 'mod-transmog: Shattrath, Terrace of Light');
