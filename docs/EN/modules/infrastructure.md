# Module: `FirelandsInfrastructure` (`src/infrastructure`)

## Role

**Infrastructure** wires the emulator to the outside world: **MariaDB**, **Boost.Asio** TCP servers, optional **REST**, **realm-link** TCP between auth and world, **Lua** scripting, and **collision** stubs. It implements domain/application repository interfaces and session handlers.

## Persistence (`infrastructure/persistence/`)

- **`DatabaseMigrator`** — runs ordered `.sql` files from the repo’s `sql/` directory, tracks applied migrations in `schema_migrations`, splits statements safely.
- **`DatabaseService`** — connection/helper patterns as used by tooling or servers.
- **`MySqlAccountRepository`** — `IAccountRepository` against the auth DB (accounts, session keys, account_data).
- **`MySqlRealmRepository`** — `IRealmRepository` for `realmlist` (and related) rows.
- **`MySqlCharacterRepository`** — `ICharacterRepository` for the characters database.
- **`MySqlPlayerCreateInfoRepository`** — world DB static templates for new characters.
- **`MemoryWebSessionRepository`** — ephemeral REST sessions for Dev/API flows.

## Network (`infrastructure/network/`)

- **`AsyncNetworkServer`** — accept loop + session factory; `Update()` polled from `main`.
- **`AuthSession`** — WoW auth protocol handling; uses `AuthService`, `RealmListService`.
- **`WorldSession`** — world socket protocol for logged-in clients; uses `AuthService`, `CharacterService`, `PlayerCreateInfoService`, `ICommandService`.
- **`RestAuthServer`** — HTTP API alongside classic auth (ported services injected).
- **Realm link** — `RealmLiveRegistry` + `RealmLinkSession` on **auth** accept authenticated realm status from **world** (`RealmLinkOutbound` connects out from world using `RealmLinkProtocol` in shared).

## Scripting & world adapters

- **`LuaGameScriptHost`** — loads Lua under `Scripting.ScriptsDirectory`; fires events (`world_startup`, spawn hooks, etc.) via `IGameScriptHost`.
- **`MapCollisionQueriesStub`** — placeholder `IMapCollisionQueries` implementation gated by config paths until full vmap integration.

## CMake

`FirelandsInfrastructure` links **FirelandsApplication**, MariaDB C++, Boost thread, nlohmann_json, **Lua**, zlib; includes connector headers. `LuaGameScriptHost.cpp` skips PCH for toolchain compatibility.

