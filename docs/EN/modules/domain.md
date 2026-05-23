# Module: `FirelandsDomain` (`src/domain`)

## Role

The **domain** layer models core game and account concepts and defines **repository interfaces** (ports). It avoids infrastructure details: no SQL, no sockets.

## Compiled sources (`FirelandsDomain`)

`src/domain/CMakeLists.txt` builds:

- `models/Realm.cpp` — realm metadata used for realm lists and live state.
- `world/Map.cpp` — map grid / object indexing for `WorldObject` descendants.
- `world/Creature.cpp`, `world/GameObject.cpp` — world entities with placement and identity.

## Header-only areas (included by application/infrastructure)

- **Models** — e.g. `Character`, `PlayerCreateInfo`, `Account` types (via repositories), `WebSession`, `Chat`, `Realm` declarations as appropriate.
- **World entities** — `Player`, `WorldObject`, `Unit`, `Map` API used by `WorldService` and sessions.
- **Repository interfaces** — `IAccountRepository`, `IRealmRepository`, `ICharacterRepository`, `IPlayerCreateInfoRepository`, `IWebSessionRepository`: contracts implemented in `MySql*` classes under infrastructure.
- **Ports** (`domain/ports/`) — e.g. `IMapNotifier`: callbacks from `Player` on a map to the owning session (packets, kill notifications) without domain importing infrastructure.

## Principles

- Domain code expresses **what** the emulator manipulates (accounts, realms, characters, maps, creatures), not **how** data is stored or packets are sent.
- New gameplay rules that only need pure domain types can live here; anything tied to SQL or packet parsing belongs in infrastructure or shared network helpers.

