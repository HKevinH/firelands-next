#!/usr/bin/env python3
"""
Import Cataclysm phase `conditions` rows (source type 26) into Firelands Next.

Reference: firelands-cata-ref `conditions.sql` — gates `phase_area` by quest/aura progress.

Emits JDBC-safe world migration:
  USE `firelands_world`;
  DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 26;
  batched REPLACE INTO ...

Requires migration 56_world_conditions.sql (DDL).

Usage:
  python3 tools/sql/import_ref_phase_conditions.py
  python3 tools/sql/import_ref_phase_conditions.py --ref /path/to/firelands-cata-ref \\
      --out sql/migrations/57_world_phase_conditions_data.sql
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

_REPO_ROOT = Path(__file__).resolve().parents[2]
_TOOLS_SQL = Path(__file__).resolve().parent
if str(_TOOLS_SQL) not in sys.path:
    sys.path.insert(0, str(_TOOLS_SQL))

from import_ref_creature_data import extract_insert_rows, write_batched  # noqa: E402

CONDITION_SOURCE_TYPE_PHASE = 26

CONDITION_COLUMNS = (
    "`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, "
    "`ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, "
    "`ConditionValue1`, `ConditionValue2`, `ConditionValue3`, "
    "`NegativeCondition`, `ErrorType`, `ErrorTextId`, `ScriptName`, `Comment`"
)


def is_phase_condition_row(fields: list[str]) -> bool:
    if not fields:
        return False
    return fields[0].strip() == str(CONDITION_SOURCE_TYPE_PHASE)


def map_condition_row(fields: list[str]) -> str:
    if len(fields) < 15:
        raise ValueError(f"conditions row expected >= 15 fields, got {len(fields)}")
    return "(" + ",".join(fields[:15]) + ")"


def write_phase_conditions_migration(conditions_sql: Path, out_path: Path) -> None:
    all_fields = extract_insert_rows(conditions_sql, "conditions")
    phase_rows = [map_condition_row(f) for f in all_fields if is_phase_condition_row(f)]

    header = (
        "-- Phase area conditions (quest/aura gates for zone phasing).\n"
        "-- Reference: firelands-cata-ref `conditions` (SourceType 26).\n"
        "-- JDBC-safe: DELETE type-26 rows + batched REPLACE.\n"
        "-- Regenerate: python3 tools/sql/import_ref_phase_conditions.py\n"
        "-- Requires migration 56 (conditions DDL).\n"
        "\n"
        "USE `firelands_world`;\n"
        "\n"
        f"DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = {CONDITION_SOURCE_TYPE_PHASE};\n"
        "\n"
    )

    out_path.parent.mkdir(parents=True, exist_ok=True)
    with out_path.open("w", encoding="utf-8") as out:
        out.write(header)
        if phase_rows:
            write_batched(
                out,
                f"REPLACE INTO `conditions` ({CONDITION_COLUMNS}) VALUES",
                phase_rows,
                24,
            )

    mib = out_path.stat().st_size / 1024
    print(f"Wrote {out_path.name}: phase conditions={len(phase_rows)} ({mib:.1f} KiB)")


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument(
        "--ref",
        type=Path,
        default=_REPO_ROOT / "firelands-cata-ref",
        help="Path to firelands-cata-ref checkout",
    )
    ap.add_argument(
        "--out",
        type=Path,
        default=_REPO_ROOT / "sql" / "migrations" / "57_world_phase_conditions_data.sql",
        help="Output migration SQL path",
    )
    args = ap.parse_args()

    conditions_sql = args.ref / "data" / "sql" / "base" / "db_world" / "conditions.sql"
    if not conditions_sql.is_file():
        raise SystemExit(f"Missing {conditions_sql}")

    print(f"Parsing {conditions_sql.name} ...")
    write_phase_conditions_migration(conditions_sql, args.out)


if __name__ == "__main__":
    main()
