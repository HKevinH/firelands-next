# Parity matrix: firelands-next vs firelands-cata-ref

Living document: update **Status** and **Next criterion** as work lands.  
Reference tree: `firelands-cata-ref/` (FirelandsCore 4.3.4).

| Subsystem | Reference (primary paths) | firelands-next | Status | Next criterion |
|-----------|---------------------------|----------------|--------|----------------|
| Auth / SRP | `authserver`, `SRP6` | `AuthSession`, `SRPService` | Done | Client login stable |
| Realm list | `RealmList`, handlers | `RealmListService`, `AuthSession` | Done | Match packet fields vs ref |
| World socket / crypto | `WorldSocket`, `WorldCrypt` | `WorldSession`, `WorldCrypt` | Partial | Full header edge cases vs ref |
| Opcodes / packets | `Opcodes.cpp`, `Packets/` | `WorldOpcodes.h`, `WorldPacket`, handlers | Partial | Opcode coverage matrix per login + world |
| Character DB / enum | `CharacterDatabase`, handlers | `MySqlCharacterRepository`, `CharacterService` | Done | Schema parity with ref SQL |
| Player login sequence | `CharacterHandler`, `Player::SendInitialPackets*` | `WorldSession::HandlePlayerLogin` | Partial | Byte-for-byte spot-check critical SMSG vs ref |
| Movement | `MovementHandler`, `Map` | `HandleMovement`, `Map::UpdateObjectPosition` | Partial | Validate opcode filter + anti-cheat hooks |
| Map / grid | `Map`, `Grid`, `Object` | `Map`, `WorldObject`, `Player` | Partial | Multi-map instance IDs (see Instances) |
| Visibility / broadcast | `Map::SendToPlayers`, grid visibility | `BroadcastPacket`, `BroadcastPacketToNearby` | Partial | SAY/YELL nearby (implemented); true visibility range later |
| Chat | `ChatHandler` | `HandleMessageChat` | Partial | Guild/party/whisper parity |
| Scripting / hooks | `ScriptMgr`, AI scripts | `IGameScriptHost`, `LuaGameScriptHost`, Lua `OnScriptEvent` | Partial | Expand C++→Lua surface + sandbox |
| Creatures / GOs | `Creature`, `GameObject`, spawns | `Creature`, `GameObject` domain types | Started | Spawn pipeline + `SMSG_UPDATE_OBJECT` for units |
| Combat / spells | `Spell`, `Unit`, `Aura` | — | Not started | One cast + GCD + aura stub |
| Quests / gossip | `QuestHandler`, `NPCHandler` | `CMSG_GOSSIP_*` → Lua `gossip_*` events | Started | SMSG_GOSSIP_MESSAGE + menu state |
| Loot | `LootMgr`, `Loot` | — | Not started | Basic take-item flow |
| Collision / path | `VMap`, `MMap`, `MapInstanced` | `IMapCollisionQueries` + stub | Started | Wire `Collision.DataRoot` to real queries |
| Instances / phases | `InstanceMap`, `InstanceScript` | — | Not started | Instance id on `Map` + reset hooks |
| Data stores (DBC) | `DB2Store`, SQL | — | Not started | Load critical templates for spells/units |
| Battlegrounds / arena | `Battleground*` | — | Not started | Out of scope until open world stable |
| Anticheat | `Anticheat` | — | Not started | Movement validation baseline |

**Priority order (short term):** world opcodes + visibility → creatures on map → combat stub → quests/gossip SMSG → collision data → instances.

See also [PARITY_AND_LUA_ROADMAP.md](PARITY_AND_LUA_ROADMAP.md) and [implementation_plan.md](../implementation_plan.md).
