# Module: `FirelandsShared` (`src/shared`)

## Role

`FirelandsShared` is the lowest-level **static library** used by every other target. It holds cross-cutting utilities that must not depend on game services or persistence:

- **Configuration** — `Config.cpp` / `Config.h` load YAML via yaml-cpp, with search paths (cwd, executable directory, `FIRELANDS_AUTH_CONFIG` / `FIRELANDS_WORLD_CONFIG`). Nested keys and typed helpers (`GetNested`, `GetNestedScalarString`) support server YAML files.
- **Logging** — `Logger.h` wraps spdlog with builders (console, file, rotation) and log levels integrated with YAML (`LogLevel` conversions).
- **Networking helpers** — Headers under `shared/network/` define WoW-style constructs: `ByteBuffer`, `WorldPacket`, opcode constants (`WorldOpcodes.h`), auth/world packet layouts (`AuthPackets.h`, server packets), encryption (`WorldCrypt.h`), movement/update field helpers, and wire codecs such as `SpellCastWire`.
- **Cryptography / SRP support** — `Crypto.h`, `SRPConstants.h`, `BigInt.h` support authentication math aligned with the emulator’s SRP flow (used together with application-layer `SRPService`).
- **DBC reading** — `dbc/DbcReader.cpp` reads client `.dbc`-style binary tables for tooling or runtime data (shared with world-related features).
- **Common definitions** — `Common.h`, `Banner.h` for shared types and CLI banners.

## CMake

Defined in `src/shared/CMakeLists.txt`: links **OpenSSL**, **spdlog**, **nlohmann_json**, **yaml-cpp**. Sources compiled today: `Config.cpp`, `network/SpellCastWire.cpp`, `dbc/DbcReader.cpp`; many other headers are header-only and included by domain/application/infrastructure.

## When to add code here

Add to **shared** only when the code is reusable, has **no** dependency on MariaDB, Boost ASIO session classes, or domain aggregates—and when multiple layers need it.

