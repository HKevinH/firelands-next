#pragma once

#include <shared/game/AccessLevel.h>
#include <shared/Common.h>
#include <cstdint>
#include <string>

namespace Firelands {

struct MovementInfo;

/// Minimal surface used by `ICommandService` / `CommandService` (in-game client or
/// server REPL).
class ICommandSession {
public:
  virtual ~ICommandSession() = default;
  virtual void SendNotification(const std::string &message) = 0;
  /// Center-screen banner (`SMSG_NOTIFICATION`); no-op for console / stubs.
  virtual void SendScreenNotification(std::string const &message) {
    (void)message;
  }
  virtual const MovementInfo &GetPosition() const = 0;
  virtual uint32 GetMapId() const { return 0; }
  virtual void TeleportTo(uint32_t mapId, float x, float y, float z,
                          float orientation = 0.0f) = 0;
  virtual AccessLevel GetAccountAccessLevel() const = 0;

  /// Graceful disconnect (e.g. `.kick`); no-op for console stub.
  virtual void RequestDisconnect(std::string const &reason) { (void)reason; }

  /// Gameplay GM helpers (no-op / false unless `WorldSession`).
  virtual bool GmLearnSpell(uint32 spellId) {
    (void)spellId;
    return false;
  }
  virtual bool GmModifyMoneyCopper(int64 deltaCopper) {
    (void)deltaCopper;
    return false;
  }
  virtual bool GmAddItem(uint32 itemEntry, uint32 count) {
    (void)itemEntry;
    (void)count;
    return false;
  }
  virtual bool GmSetLevel(uint8 level) {
    (void)level;
    return false;
  }

  /// Optional GM tooling (no-op for console stub / non-world sessions).
  virtual void SetGmTagEnabled(bool on) { (void)on; }
  virtual void SetDndEnabled(bool on) { (void)on; }
  virtual void SetDevTagEnabled(bool on) { (void)on; }
  virtual void SetGmVisibleToPlayers(bool visible) { (void)visible; }
  virtual void SetGmFlyEnabled(bool on) { (void)on; }
  virtual void SetGmRunSpeed(float speed) { (void)speed; }
};

} // namespace Firelands
