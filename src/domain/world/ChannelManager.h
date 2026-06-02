#pragma once

#include <cctype>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Firelands {

/// Process-wide registry of chat-channel membership (the WoW city/zone channels
/// like "General - Orgrimmar", "Trade - City", "LocalDefense", ...). In WoW the
/// CLIENT drives the joins/leaves per zone (it sends CMSG_JOIN_CHANNEL itself),
/// so this only needs to: track who is in each channel, and let the session layer
/// broadcast messages to the members. Channels are keyed by (faction team,
/// lowercased name) so Horde and Alliance never share a channel instance.
///
/// Pure membership bookkeeping — no packet / session knowledge here (that lives in
/// the WorldSession handlers). Thread-safe: joins/leaves come from network threads.
class ChannelManager {
public:
  static ChannelManager &Instance() {
    static ChannelManager instance;
    return instance;
  }

  struct JoinResult {
    bool alreadyMember = false;  ///< player was already in the channel
    bool firstMember = false;    ///< channel was just created by this join
    std::string displayName;     ///< channel name as first seen (original case)
  };

  /// A channel a player belongs to (used to re-bucket zone channels on zone change).
  struct PlayerChannel {
    std::string name;
    uint32_t channelId = 0;
  };

  JoinResult Join(uint8_t team, std::string const &name, uint64_t guid,
                  uint32_t channelId = 0) {
    JoinResult result;
    if (name.empty())
      return result;
    std::lock_guard<std::mutex> lock(_mutex);
    auto &ch = _channels[Key(team, name)];
    if (ch.displayName.empty()) {
      ch.displayName = name;
      result.firstMember = true;
    }
    if (channelId != 0)
      ch.channelId = channelId;  // remember the dbc channel id for notices
    result.displayName = ch.displayName;
    result.alreadyMember = !ch.members.insert(guid).second;
    return result;
  }

  /// All channels (for the player's team) the player is currently a member of.
  std::vector<PlayerChannel> ChannelsForPlayer(uint8_t team,
                                               uint64_t guid) const {
    std::vector<PlayerChannel> out;
    std::string const prefix = TeamPrefix(team);
    std::lock_guard<std::mutex> lock(_mutex);
    for (auto const &[key, ch] : _channels) {
      if (key.rfind(prefix, 0) != 0)
        continue;
      if (ch.members.count(guid) == 0)
        continue;
      out.push_back({ch.displayName, ch.channelId});
    }
    return out;
  }

  /// Returns the channel display name if the player was a member (now removed).
  std::optional<std::string> Leave(uint8_t team, std::string const &name,
                                   uint64_t guid) {
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _channels.find(Key(team, name));
    if (it == _channels.end() || it->second.members.erase(guid) == 0)
      return std::nullopt;
    std::string const display = it->second.displayName;
    if (it->second.members.empty())
      _channels.erase(it);
    return display;
  }

  std::vector<uint64_t> Members(uint8_t team, std::string const &name) const {
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _channels.find(Key(team, name));
    if (it == _channels.end())
      return {};
    return {it->second.members.begin(), it->second.members.end()};
  }

  /// Remove a player from every channel (logout / world exit). Returns the
  /// display names the player was in (for leave notices, if desired).
  std::vector<std::string> LeaveAll(uint64_t guid) {
    std::vector<std::string> left;
    std::lock_guard<std::mutex> lock(_mutex);
    for (auto it = _channels.begin(); it != _channels.end();) {
      if (it->second.members.erase(guid) > 0)
        left.push_back(it->second.displayName);
      if (it->second.members.empty())
        it = _channels.erase(it);
      else
        ++it;
    }
    return left;
  }

private:
  struct Channel {
    std::string displayName;
    uint32_t channelId = 0;
    std::unordered_set<uint64_t> members;
  };

  static std::string TeamPrefix(uint8_t team) {
    return std::string(1, static_cast<char>('0' + (team & 0x0Fu))) + ":";
  }

  static std::string Key(uint8_t team, std::string const &name) {
    std::string key = TeamPrefix(team);
    for (unsigned char c : name)
      key.push_back(static_cast<char>(std::tolower(c)));
    return key;
  }

  mutable std::mutex _mutex;
  std::unordered_map<std::string, Channel> _channels;
};

} // namespace Firelands
