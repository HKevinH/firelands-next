#!/usr/bin/env python3
"""Tests for `import_ref_creature_data.map_creature_row` phase column mapping."""

from __future__ import annotations

import importlib.util
import sys
import unittest
from pathlib import Path

_SCRIPT = Path(__file__).resolve().parent / "import_ref_creature_data.py"
_spec = importlib.util.spec_from_file_location("import_ref_creature_data", _SCRIPT)
_mod = importlib.util.module_from_spec(_spec)
assert _spec.loader is not None
sys.modules[_spec.name] = _mod
_spec.loader.exec_module(_mod)

map_creature_row = _mod.map_creature_row


class MapCreatureRowTests(unittest.TestCase):
    def test_echo_isles_quest_bunny_keeps_phase_id_and_group(self) -> None:
        """Spawn 308730 (38003) must not swap PhaseId with PhaseGroup (169 is default)."""
        ref_row = [
            "308730",
            "38003",
            "1",
            "14",
            "4867",
            "1",
            "0",
            "1",
            "169",
            "0",
            "-1",
            "0",
            "0",
            "-703.763",
            "-5573.99",
            "28.6805",
            "4.13643",
            "300",
            "0",
            "0",
            "1",
            "0",
            "0",
            "0",
            "0",
            "0",
            "''",
            "0",
        ]
        mapped = map_creature_row(ref_row)
        self.assertIn(",1,169,0,0,-1,", mapped)
        self.assertNotIn(",169,0,0,0,-1,", mapped)


if __name__ == "__main__":
    unittest.main()
