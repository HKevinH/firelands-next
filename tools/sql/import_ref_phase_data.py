#!/usr/bin/env python3
"""
Import Cataclysm phase catalog data into Firelands Next world migrations.

Sources (idea + data from firelands-cata-ref / client 4.3.4.15595):
  - `phase_area` rows from reference `phase_area.sql` (zone → default player phases)
  - `phase_x_phase_group` rows from client `PhaseXPhaseGroup.dbc` (spawn PhaseGroup resolution)

Emits a JDBC-safe world migration:
  USE `firelands_world`;
  DELETE FROM `phase_x_phase_group`;
  DELETE FROM `phase_area`;
  batched REPLACE INTO ...

Requires DDL migrations 53_world_phase_x_phase_group.sql and 54_world_phase_area.sql.

Usage:
  python3 tools/sql/import_ref_phase_data.py
  python3 tools/sql/import_ref_phase_data.py --ref /path/to/firelands-cata-ref \\
      --dbc /path/to/data/dbc/PhaseXPhaseGroup.dbc \\
      --out sql/migrations/55_world_phase_catalog_data.sql
"""

from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path

_REPO_ROOT = Path(__file__).resolve().parents[2]
_TOOLS_SQL = Path(__file__).resolve().parent
if str(_TOOLS_SQL) not in sys.path:
    sys.path.insert(0, str(_TOOLS_SQL))

from import_ref_creature_data import (  # noqa: E402
    extract_insert_rows,
    sql_escape_literal,
    strip_sql_string,
    write_batched,
)

PHASE_AREA_COLUMNS = "`AreaId`, `PhaseId`, `Comment`"
PHASE_GROUP_COLUMNS = "`ID`, `PhaseID`, `PhaseGroupID`"


def map_phase_area_row(fields: list[str]) -> str:
    if len(fields) < 2:
        raise ValueError(f"phase_area row expected >= 2 fields, got {len(fields)}")
    area_id = fields[0].strip()
    phase_id = fields[1].strip()
    comment_sql = "NULL"
    if len(fields) >= 3:
        tok = fields[2].strip()
        if tok.upper() != "NULL":
            if tok.startswith("'"):
                comment_sql = "N'" + sql_escape_literal(strip_sql_string(tok)) + "'"
            else:
                comment_sql = tok
    return f"({area_id},{phase_id},{comment_sql})"


def read_phase_x_phase_group_dbc(dbc_path: Path) -> list[tuple[int, int, int]]:
    data = dbc_path.read_bytes()
    if len(data) < 20 or data[:4] != b"WDBC":
        raise ValueError(f"Not a WDBC file: {dbc_path}")

    record_count, field_count, record_size = struct.unpack_from("<III", data, 4)
    if field_count != 3 or record_size != 12:
        raise ValueError(
            f"Unexpected PhaseXPhaseGroup.dbc layout: fields={field_count} "
            f"record_size={record_size} (expected 3 and 12)"
        )

    offset = 20
    rows: list[tuple[int, int, int]] = []
    for i in range(record_count):
        base = offset + i * record_size
        row_id, phase_id, group_id = struct.unpack_from("<III", data, base)
        if phase_id == 0:
            continue
        rows.append((row_id, phase_id, group_id))
    return rows


def map_phase_group_row(row_id: int, phase_id: int, group_id: int) -> str:
    return f"({row_id},{phase_id},{group_id})"


def write_phase_catalog_migration(
    phase_area_sql: Path,
    phase_group_dbc: Path,
    out_path: Path,
    *,
    area_batch_size: int = 28,
    group_batch_size: int = 200,
) -> None:
    area_fields = extract_insert_rows(phase_area_sql, "phase_area")
    area_rows = [map_phase_area_row(f) for f in area_fields]

    group_tuples = read_phase_x_phase_group_dbc(phase_group_dbc)
    group_rows = [map_phase_group_row(*t) for t in group_tuples]

    header = (
        "-- Phase catalog data (zone phasing + phase group membership).\n"
        "-- Reference: firelands-cata-ref `phase_area`; client PhaseXPhaseGroup.dbc.\n"
        "-- JDBC-safe: DELETE + batched REPLACE (re-runnable).\n"
        "-- Regenerate: python3 tools/sql/import_ref_phase_data.py\n"
        "-- Requires migrations 53 (phase_x_phase_group DDL) and 54 (phase_area DDL).\n"
        "\n"
        "USE `firelands_world`;\n"
        "\n"
        "DELETE FROM `phase_x_phase_group`;\n"
        "DELETE FROM `phase_area`;\n"
        "\n"
    )

    out_path.parent.mkdir(parents=True, exist_ok=True)
    with out_path.open("w", encoding="utf-8") as out:
        out.write(header)
        if area_rows:
            write_batched(
                out,
                f"REPLACE INTO `phase_area` ({PHASE_AREA_COLUMNS}) VALUES",
                area_rows,
                area_batch_size,
            )
        if group_rows:
            write_batched(
                out,
                f"REPLACE INTO `phase_x_phase_group` ({PHASE_GROUP_COLUMNS}) VALUES",
                group_rows,
                group_batch_size,
            )

    mib = out_path.stat().st_size / (1024 * 1024)
    print(
        f"Wrote {out_path.name}: phase_area={len(area_rows)} "
        f"phase_x_phase_group={len(group_rows)} ({mib:.2f} MiB)"
    )


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument(
        "--ref",
        type=Path,
        default=_REPO_ROOT / "firelands-cata-ref",
        help="Path to firelands-cata-ref checkout",
    )
    ap.add_argument(
        "--dbc",
        type=Path,
        default=_REPO_ROOT / "data" / "dbc" / "PhaseXPhaseGroup.dbc",
        help="Path to client PhaseXPhaseGroup.dbc (4.3.4.15595)",
    )
    ap.add_argument(
        "--out",
        type=Path,
        default=_REPO_ROOT / "sql" / "migrations" / "55_world_phase_catalog_data.sql",
        help="Output migration SQL path",
    )
    args = ap.parse_args()

    phase_area_sql = args.ref / "data" / "sql" / "base" / "db_world" / "phase_area.sql"
    if not phase_area_sql.is_file():
        raise SystemExit(f"Missing {phase_area_sql}")
    if not args.dbc.is_file():
        raise SystemExit(
            f"Missing {args.dbc} — extract client DBCs to data/dbc (see worldserver.yaml DbcPath)"
        )

    print(f"Parsing {phase_area_sql.name} ...")
    print(f"Parsing {args.dbc.name} ...")
    write_phase_catalog_migration(phase_area_sql, args.dbc, args.out)


if __name__ == "__main__":
    main()
