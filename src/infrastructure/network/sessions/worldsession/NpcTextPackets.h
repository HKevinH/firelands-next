#pragma once

#include <shared/network/WorldOpcodes.h>
#include <shared/network/WorldPacket.h>
#include <cstdint>
#include <string>

namespace Firelands::gossip {

/// Trinity `MAX_GOSSIP_TEXT_OPTIONS` / `MAX_GOSSIP_TEXT_EMOTES` (Cataclysm).
inline constexpr uint8_t kNpcTextOptionCount = 8;
inline constexpr uint8_t kNpcTextEmoteCount = 3;

/// Build `SMSG_NPC_TEXT_UPDATE` — matches TCPP `HandleNpcTextQueryOpcode` fallback layout.
inline WorldPacket BuildNpcTextUpdate(uint32_t textId,
                                      std::string const &greeting = "Greetings $N") {
  WorldPacket out(SMSG_NPC_TEXT_UPDATE, 256);
  out.Append<uint32_t>(textId);

  for (uint8_t i = 0; i < kNpcTextOptionCount; ++i) {
    out.Append<float>(i == 0 ? 1.0f : 0.0f);
    out.WriteString(greeting);
    out.WriteString(greeting);
    out.Append<uint32_t>(0); // language
    for (uint8_t j = 0; j < kNpcTextEmoteCount; ++j) {
      out.Append<uint32_t>(0); // emote delay
      out.Append<uint32_t>(0); // emote id
    }
  }
  return out;
}

} // namespace Firelands::gossip
