# Client data extractors (4.3.4 / MPQ)

Tools read a retail **Cataclysm** `World of Warcraft/Data` folder (MPQ archives, not CASC) and extract files using **StormLib v9.26** (pinned in the root `CMakeLists.txt`).

## Prerequisites

- CMake build of the project (same toolchain as the rest of Firelands).
- System **zlib** and **bzip2** development packages (StormLib links them when `STORM_USE_BUNDLED_LIBRARIES` is off).

## Binaries

| Target | Purpose |
|--------|---------|
| **`firelands-extractors`** | **Fullscreen TUI launcher** (FTXUI): Firelands banner, pick task (DBC/DB2, maps, list MPQs), edit paths, run — output streams into an on-screen console. |
| `firelands-dbc-extractor` | Extracts `DBFilesClient\*.dbc` and `DBFilesClient\*.db2`; requires **`--data`** / **`--out`** (or **`--list-mpqs`**); **`--help`** prints usage. |
| `firelands-map-extractor` | Map asset extraction; same CLI contract as the DBC tool (**no** interactive menu when run bare). |

Build artifacts land under `${CMAKE_BINARY_DIR}/bin/` (see your generator output).

## TUI launcher (`firelands-extractors`)

Run with **no arguments** from an interactive terminal (TTY):

```bash
./firelands-extractors
```

Choose the operation, fill **WoW `Data`** and (unless listing MPQs only) **output folder**, then **Run**. Scroll output with **PgUp/PgDn** or the mouse wheel; **Q** exits when idle. Without a TTY (CI/pipes), the tool exits with an error — use the dedicated binaries below.

`firelands-extractors --help` summarizes scripted usage.

## CLI-only mode (scripts / CI)

List the MPQ open order StormLib will use (base archive + patches):

```bash
./firelands-dbc-extractor --data "/path/to/WoW/Data" --list-mpqs
```

Extract all `DBFilesClient` tables (`.dbc` and `.db2`):

```bash
./firelands-dbc-extractor --data "/path/to/WoW/Data" --out ./client-dbc
```

Extract map-related client files:

```bash
./firelands-map-extractor --data "/path/to/WoW/Data" --out ./client-maps
```

## MPQ ordering

Archives under `Data` are sorted for **patch overlay** order: the first file in the printed list is opened as the base archive; each following file is applied with `SFileOpenPatchArchive` so later entries override earlier ones for the same internal path. Details and milestones: **`STORM_LIB_ROADMAP.md`**.

## See also

- `STORM_LIB_ROADMAP.md` — versions, risks, and phase-2 server map tooling.
- Official StormLib: [ladislav-zezula/StormLib](https://github.com/ladislav-zezula/StormLib).
