#include <algorithm>
#include <ctime>
#include <domain/models/Chat.h>
#include <infrastructure/network/sessions/WorldSession.h>
#include <random>
#include <shared/Crypto.h>
#include <shared/Logger.h>
#include <shared/network/UpdateData.h>
#include <shared/network/UpdateFields.h>

namespace Firelands {

WorldSession::WorldSession(tcp::socket socket,
                           std::shared_ptr<AuthService> authService,
                           std::shared_ptr<CharacterService> charService,
                           std::shared_ptr<ICommandService> commandService)
    : _socket(std::move(socket)), _authService(std::move(authService)),
      _charService(std::move(charService)), _commandService(std::move(commandService)),
      _serverSeed(0), _accountId(0) {}

void WorldSession::Start() {
  LOG_INFO("WorldSession started for {}", GetIpAddress());

  // Cataclysm 4.3.4 Handshake: Server sends initializer string first (NO
  // OPCODES)
  std::string initializer = "WORLD OF WARCRAFT CONNECTION - SERVER TO CLIENT";
  ByteBuffer buffer;

  // Header for the initializer: just [Size:2 (BE)], followed by the string
  // payload.
  uint16 size = static_cast<uint16>(initializer.length());
  buffer.Append<uint8>((size >> 8) & 0xFF);
  buffer.Append<uint8>(size & 0xFF);
  buffer.Append((const uint8 *)initializer.c_str(), initializer.length());

  SendPacket(buffer);
  DoRead();
}

void WorldSession::SendPacket(WorldPacket &packet) {
  ByteBuffer buffer;
  // En Cataclysm, la cabecera del servidor es [Size:2 (BE)][Opcode:2 (LE)]
  // El Size incluye los 2 bytes del Opcode.
  uint16 size = static_cast<uint16>(packet.Size() + 2);

  uint8 header[4];
  header[0] = (size >> 8) & 0xFF;
  header[1] = size & 0xFF;
  uint16 opcode = static_cast<uint16>(packet.GetOpcode());
  header[2] = opcode & 0xFF;
  header[3] = (opcode >> 8) & 0xFF;

  // En Cataclysm 4.3.4, TODA la cabecera de 4 bytes se encripta
  _crypt.EncryptSend(header, 4);

  buffer.Append(header, 4);
  if (packet.Size() > 0) {
    buffer.Append(packet.GetBuffer(), packet.Size());
  }

  SendPacket(buffer);
}

void WorldSession::SendPacket(ByteBuffer &buffer) {
  auto shared_buffer = std::make_shared<std::vector<uint8>>(
      buffer.GetBuffer(), buffer.GetBuffer() + buffer.Size());

  // Log hex of every outgoing packet for diagnostics
  {
    std::string hexDump;
    for (size_t i = 0; i < shared_buffer->size() && i < 64; ++i) {
      char hex[4];
      std::snprintf(hex, sizeof(hex), "%02X ", (*shared_buffer)[i]);
      hexDump += hex;
    }
    if (shared_buffer->size() > 64)
      hexDump += "...";
    LOG_INFO("[SEND] {} bytes: {}", shared_buffer->size(), hexDump);
  }

  // Queue the buffer and start writing if not already in progress
  _writeQueue.push_back(shared_buffer);
  if (!_writing) {
    DoWrite();
  }
}

void WorldSession::DoWrite() {
  if (_writeQueue.empty()) {
    _writing = false;
    return;
  }

  _writing = true;
  auto self(shared_from_this());
  auto buffer = _writeQueue.front();
  _writeQueue.pop_front();

  boost::asio::async_write(
      _socket, boost::asio::buffer(buffer->data(), buffer->size()),
      [this, self, buffer](boost::system::error_code ec,
                           std::size_t /*length*/) {
        if (ec) {
          LOG_ERROR("[SEND] Write error: {}", ec.message());
          Close();
          return;
        }
        // Process next queued packet
        DoWrite();
      });
}

void WorldSession::SendAuthChallenge() {
  _serverSeed = static_cast<uint32>(std::rand());

  WorldPacket data(SMSG_AUTH_CHALLENGE);

  // Cata 4.3.4 (15595) SMSG_AUTH_CHALLENGE (37 bytes):
  // Estructura exacta de firelands-cata-ref:
  // [32] DosChallenge (con sobreescritura parcial)
  // [4]  Server Seed (uint32)
  // [1]  DosZeroBits (uint8)

  uint8 dosChallenge[32];
  std::memset(dosChallenge, 0, 32);

  uint8 encryptSeed[16], decryptSeed[16];
  for (int i = 0; i < 16; ++i) {
    encryptSeed[i] = static_cast<uint8>(std::rand() % 256);
    decryptSeed[i] = static_cast<uint8>(std::rand() % 256);
  }

  // Replicamos el memcpy solapado de la referencia:
  std::memcpy(&dosChallenge[0], encryptSeed, 16);
  std::memcpy(&dosChallenge[4], decryptSeed, 16); // Sobreescribe índices 4-19

  data.Append(dosChallenge, 32);
  data.Append<uint32>(_serverSeed);
  data.Append<uint8>(1);

  SendPacket(data);
  LOG_INFO("SMSG_AUTH_CHALLENGE sent (ServerSeed: 0x{:08X})", _serverSeed);
}
void WorldSession::Close() {
  if (_socket.is_open()) {
    LOG_INFO("Closing WorldSession for {}", GetIpAddress());
    _socket.close();
  }
}

std::string WorldSession::GetIpAddress() const {
  try {
    return _socket.remote_endpoint().address().to_string();
  } catch (...) {
    return "unknown";
  }
}

void WorldSession::DoRead() {
  auto self(shared_from_this());
  _socket.async_read_some(
      boost::asio::buffer(_readBuffer, sizeof(_readBuffer)),
      [this, self](boost::system::error_code ec, std::size_t length) {
        if (!ec) {

          _inBuffer.Append(_readBuffer, length);

          // Process as many packets as possible from the buffer
          while (true) {
            if (!_initialized) {
              // Handshake: unencrypted [Size:2 BE][String]
              if (_inBuffer.Size() < 2)
                break;
              uint16 size = (_inBuffer[0] << 8) | _inBuffer[1];
              if (_inBuffer.Size() < static_cast<size_t>(size + 2))
                break;

              ByteBuffer packetData;
              packetData.Append(_inBuffer.GetBuffer(), size + 2);
              HandlePacket(packetData);

              std::vector<uint8> remaining(_inBuffer.GetBuffer() + size + 2,
                                           _inBuffer.GetBuffer() +
                                               _inBuffer.Size());
              _inBuffer.Clear();
              _inBuffer.Append(remaining.data(), remaining.size());
              continue;
            }

            // Post-init: CMSG header = 6 bytes [Size:2 BE][Opcode:4 LE]
            // These 6 bytes may be ARC4-encrypted.
            if (_inBuffer.Size() < 6)
              break;

            // Decrypt header exactamente una vez por paquete (ARC4 tiene
            // estado)
            if (_crypt.IsInitialized() && !_headerDecrypted) {
              std::memcpy(_decHeader, _inBuffer.GetBuffer(), 6);
              // En Cataclysm 4.3.4, los 6 bytes de la cabecera CMSG están
              // encriptados
              _crypt.DecryptRecv(_decHeader, 6);
              _headerDecrypted = true;
            } else if (!_crypt.IsInitialized() && !_headerDecrypted) {
              std::memcpy(_decHeader, _inBuffer.GetBuffer(), 6);
              _headerDecrypted = true;
            }

            // In Cataclysm, the Size field includes the 4-byte Opcode
            uint16 pktSize = (_decHeader[0] << 8) | _decHeader[1];
            // Opcode is 4 bytes, LITTLE ENDIAN
            uint32 opcode = _decHeader[2] | (_decHeader[3] << 8) |
                            (_decHeader[4] << 16) | (_decHeader[5] << 24);

            LOG_INFO("[DEBUG] Decrypted Header: Size {}, Opcode 0x{:08X}",
                     pktSize, opcode);

            // Total on wire: 2 (size field) + pktSize
            if (_inBuffer.Size() < static_cast<size_t>(pktSize + 2)) {
              if (_inBuffer.Size() >= 6) {
                LOG_DEBUG("Waiting for more data. Have {}, need {}",
                          _inBuffer.Size(), pktSize + 2);
              }
              break;
            }

            _headerDecrypted = false;

            LOG_INFO("[PKT] Received: 0x{:04X} (Decrypted: {}), Size: {}",
                     opcode, _crypt.IsInitialized(), pktSize);

            uint32 payloadSize = (pktSize >= 4) ? (pktSize - 4) : 0;

            WorldPacket packet(opcode, payloadSize);
            if (payloadSize > 0) {
              packet.Append(_inBuffer.GetBuffer() + 6, payloadSize);
            }

            // Remove consumed bytes
            size_t consumed = pktSize + 2;
            std::vector<uint8> remaining(_inBuffer.GetBuffer() + consumed,
                                         _inBuffer.GetBuffer() +
                                             _inBuffer.Size());
            _inBuffer.Clear();
            _inBuffer.Append(remaining.data(), remaining.size());

            ProcessPacket(packet);
          }

          DoRead();
        } else if (ec != boost::asio::error::operation_aborted) {
          LOG_ERROR("DoRead error: {} ({})", ec.message(), ec.value());
          Close();
        }
      });
}

void WorldSession::HandlePacket(ByteBuffer &buffer) {
  // This method handles the initial handshake string.
  // The buffer passed here contains [Size:2 BE][String]
  if (buffer.Size() < 2)
    return;

  uint16 size = buffer.Read<uint16>();
  size = (size << 8) | (size >> 8);

  if (buffer.Size() < size)
    return;

  std::string expected = "WORLD OF WARCRAFT CONNECTION - CLIENT TO SERVER";
  std::string received;
  for (uint16 i = 0; i < size; ++i) {
    received += static_cast<char>(buffer.Read<uint8>());
  }

  while (!received.empty() &&
         (received.back() == '\0' || received.back() == '\r' ||
          received.back() == '\n')) {
    received.pop_back();
  }

  if (received == expected) {
    LOG_INFO("WorldSession: Handshake string validated.");
    _initialized = true;

    SendAuthChallenge();
  } else {
    LOG_ERROR("WorldSession: Invalid handshake string received. Expected '{}', "
              "got '{}'",
              expected, received);
    Close();
  }
}

void WorldSession::ProcessPacket(WorldPacket &packet) {
  uint32 opcode = packet.GetOpcode();
  LOG_INFO("WorldSession received packet: {}, size: {}", packet.GetOpcodeName(),
           packet.Size());

  switch (opcode) {
  case CMSG_AUTH_SESSION:
    HandleAuthSession(packet);
    break;
  case CMSG_CHAR_ENUM:
    HandleCharEnum(packet);
    break;
  case CMSG_CHAR_CREATE:
    HandleCharCreate(packet);
    break;
  case CMSG_CHAR_DELETE:
    HandleCharDelete(packet);
    break;
  case CMSG_PLAYER_LOGIN:
    HandlePlayerLogin(packet);
    break;
  case CMSG_MESSAGECHAT:
    HandleMessageChat(packet);
    break;
  case MSG_MOVE_HEARTBEAT:
  case MSG_MOVE_START_FORWARD:
  case MSG_MOVE_START_BACKWARD:
  case MSG_MOVE_STOP:
  case MSG_MOVE_SET_FACING:
    HandleMovement(packet);
    break;
  case CMSG_PING:
    HandlePing(packet);
    break;
  case CMSG_REALM_SPLIT:
    HandleRealmSplit(packet);
    break;
  case CMSG_READY_FOR_ACCOUNT_DATA_TIMES:
    HandleReadyForAccountDataTimes(packet);
    break;
  case CMSG_UPDATE_ACCOUNT_DATA:
    HandleUpdateAccountData(packet);
    break;
  case CMSG_LOG_DISCONNECT:
    LOG_INFO("Client disconnected (CMSG_LOG_DISCONNECT)");
    Close();
    break;
  default:
    LOG_WARN("[PACKET] Unknown/unhandled opcode: 0x{:04X} (size: {})", opcode,
             packet.Size());
    break;
  }
}

void WorldSession::HandleAuthSession(WorldPacket &packet) {
  uint8 digest[20];
  std::vector<uint8> localChallenge(4);
  uint16 build;
  uint32 realmId;
  int32 loginServerId;

  // 1. Read seeds, digest and build using the Scattered format
  HandleAuthSessionScattered(packet, digest, localChallenge, build, realmId,
                             loginServerId);

  // 2. Read addon data size and skip it
  uint32 addonDataSize = 0;
  if (packet.GetReadPos() + 4 <= packet.Size()) {
    addonDataSize = packet.Read<uint32>();
  }
  if (addonDataSize > 0 &&
      (packet.GetReadPos() + addonDataSize) <= packet.Size()) {
    packet.SetReadPos(packet.GetReadPos() + addonDataSize);
  }

  // 3. Extract Account Name using BitReader (Cataclysm 4.3.4 Build 15595)
  BitReader br(packet);
  br.ReadBit(); // UseIPv6
  uint32 accountNameLength = br.ReadBits(12);
  std::string account = br.ReadString(accountNameLength);

  LOG_INFO("CMSG_AUTH_SESSION: Account: '{}', Build: {}, RealmID: {}, "
           "ClientSeed: {}, Packet Size: {}",
           account, build, realmId,
           Crypto::ToHexString(localChallenge.data(), 4), packet.Size());

  auto accountOpt = _authService->FindAccount(account);
  if (!accountOpt) {
    LOG_ERROR("CMSG_AUTH_SESSION: Account '{}' not found.", account);
    Close();
    return;
  }

  std::vector<uint8_t> K = _authService->GetSessionKey(accountOpt->id);
  if (K.empty()) {
    LOG_ERROR("CMSG_AUTH_SESSION: No session key K for account '{}'.", account);
    Close();
    return;
  }

  // Initialize WorldCrypt IMMEDIATELY after getting K (Cataclysm requirement)
  _crypt.Init(K);
  LOG_INFO("[AUTH] WorldCrypt initialized with 40 bytes of K");

  // 4. Perform Digest validation
  // Referencia Cataclysm (WorldSocket.cpp:600):
  // SHA1(Account, t(0), ClientChallenge, ServerSeed, SessionKey)
  Crypto::SHA1 sha;
  sha.Update(Crypto::ToUpper(account));

  uint32 t = 0;
  sha.Update(t);

  sha.Update(localChallenge.data(), 4);
  sha.Update(_serverSeed);
  sha.Update(K.data(), K.size());

  Crypto::SHA1Hash calculatedDigest = sha.Finalize();

  if (std::memcmp(calculatedDigest.data(), digest, 20) != 0) {
    LOG_ERROR("CMSG_AUTH_SESSION: Digest validation failed for account '{}'!",
              account);
    LOG_INFO("Calculated: {}", Crypto::ToHexString(calculatedDigest));
    LOG_INFO("Received:   {}", Crypto::ToHexString(digest, 20));
    Close();
    return;
  }

  _accountId = accountOpt->id;
  LOG_INFO("CMSG_AUTH_SESSION: Digest validated successfully for account '{}'.",
           account);

  SendAuthResponse();
  SendAddonInfo();
}

void WorldSession::HandleAuthSessionScattered(
    WorldPacket &packet, uint8 *digest, std::vector<uint8> &localChallenge,
    uint16 &build, uint32 &realmId, int32 &loginServerId) {
  loginServerId = packet.Read<int32>();
  uint32 battlegroupId = packet.Read<uint32>();
  int8 loginServerType = packet.Read<int8>();

  digest[10] = packet.Read<uint8>();
  digest[18] = packet.Read<uint8>();
  digest[12] = packet.Read<uint8>();
  digest[5] = packet.Read<uint8>();

  uint64 dosResponse = packet.Read<uint64>();

  digest[15] = packet.Read<uint8>();
  digest[9] = packet.Read<uint8>();
  digest[19] = packet.Read<uint8>();
  digest[4] = packet.Read<uint8>();
  digest[7] = packet.Read<uint8>();
  digest[16] = packet.Read<uint8>();
  digest[3] = packet.Read<uint8>();

  build = packet.Read<uint16>();
  digest[8] = packet.Read<uint8>();

  realmId = packet.Read<uint32>();
  int8 buildType = packet.Read<int8>();

  digest[17] = packet.Read<uint8>();
  digest[6] = packet.Read<uint8>();
  digest[0] = packet.Read<uint8>();
  digest[1] = packet.Read<uint8>();
  digest[11] = packet.Read<uint8>();

  localChallenge[0] = packet.Read<uint8>();
  localChallenge[1] = packet.Read<uint8>();
  localChallenge[2] = packet.Read<uint8>();
  localChallenge[3] = packet.Read<uint8>();

  digest[2] = packet.Read<uint8>();
  uint32 regionId = packet.Read<uint32>();

  digest[14] = packet.Read<uint8>();
  digest[13] = packet.Read<uint8>();
}

void WorldSession::HandleAuthSessionStandard(WorldPacket &packet, uint16 &build,
                                             uint8 *digest,
                                             std::vector<uint8> &localChallenge,
                                             uint32 &realmId) {
  build = static_cast<uint16>(packet.Read<uint32>());
  packet.Read<uint32>(); // loginServerId or unknown
  realmId = packet.Read<uint32>();

  packet.Read(localChallenge.data(), 4);
  packet.Read<uint32>(); // Seed
  packet.Read<uint32>(); // Unk
  packet.Read<uint32>(); // Unk

  packet.Read(digest, 20);
}

void WorldSession::SendAuthResponse() {
  WorldPacket response(SMSG_AUTH_RESPONSE);

  // Cata 4.3.4 (15595) SMSG_AUTH_RESPONSE bit-packed structure
  // Referencia: AuthenticationPackets.cpp:105
  BitWriter bw(response);
  bw.WriteBit(false); // hasWaitInfo
  bw.WriteBit(true);  // hasSuccessInfo
  bw.Flush();

  // SuccessInfo fields (if hasSuccessInfo was true)
  response.Append<uint32>(0); // TimeRemain
  response.Append<uint8>(3);  // ActiveExpansionLevel (Cata)
  response.Append<uint32>(0); // TimeSecondsUntilPCKick
  response.Append<uint8>(3);  // AccountExpansionLevel (Cata)
  response.Append<uint32>(0); // TimeRested
  response.Append<uint8>(0);  // TimeOptions

  // Result (AUTH_OK = 12)
  response.Append<uint8>(12);

  SendPacket(response);
  LOG_INFO("[AUTH] SMSG_AUTH_RESPONSE (AUTH_OK) sent (Full Parity).");
}

void WorldSession::SendAddonInfo() {
  WorldPacket data(SMSG_ADDON_INFO);

  // In 15595, when no addons are sent, we just send two 0 uint32s (counts)
  data.Append<uint32>(0); // Addon count
  data.Append<uint32>(0); // Banned addon count

  SendPacket(data);
  LOG_INFO("[AUTH] SMSG_ADDON_INFO sent (Empty, 15595 parity)");
}

void WorldSession::SendClientCacheVersion() {
  WorldPacket cacheVer(SMSG_CLIENTCACHE_VERSION);
  cacheVer.Append<uint32>(0);
  SendPacket(cacheVer);
  LOG_INFO("[AUTH] SMSG_CLIENTCACHE_VERSION sent");
}

void WorldSession::SendTutorialFlags() {
  WorldPacket tutorials(SMSG_TUTORIAL_FLAGS);
  for (int i = 0; i < 8; ++i)
    tutorials.Append<uint32>(0xFFFFFFFF);
  SendPacket(tutorials);
  LOG_INFO("[AUTH] SMSG_TUTORIAL_FLAGS sent");
}

void WorldSession::SendAccountDataTimes(uint32 mask) {
  WorldPacket data(SMSG_ACCOUNT_DATA_TIMES);
  data.Append<uint32>(static_cast<uint32>(std::time(nullptr))); // Server time
  data.Append<uint8>(1);     // Unknown byte from reference
  data.Append<uint32>(mask); // type mask

  for (int i = 0; i < 8; ++i) {
    if (mask & (1 << i)) {
      data.Append<uint32>(0); // Time for each data type
    }
  }
  SendPacket(data);
  LOG_INFO("[AUTH] SMSG_ACCOUNT_DATA_TIMES sent (Full Parity, mask: 0x{:02X})",
           mask);
}

void WorldSession::SendFeatureSystemStatus() {
  WorldPacket features(SMSG_FEATURE_SYSTEM_STATUS);

  // Cata 4.3.4 (15595) Byte fields
  features.Append<uint8>(0);  // ComplaintStatus
  features.Append<uint32>(0); // ScrollOfResurrectionRequestsRemaining
  features.Append<uint32>(0); // ScrollOfResurrectionMaxRequestsPerDay
  features.Append<uint32>(0); // CfgRealmID
  features.Append<uint32>(0); // CfgRealmRecID

  // Bit fields (Exact order for 15595)
  BitWriter bw(features);
  bw.WriteBit(false); // ItemRestorationButtonEnabled
  bw.WriteBit(false); // TravelPassEnabled
  bw.WriteBit(false); // ScrollOfResurrectionEnabled
  bw.WriteBit(false); // EuropaTicketSystemStatus.has_value()
  bw.WriteBit(false); // SessionAlert.has_value()
  bw.WriteBit(false); // VoiceEnabled
  bw.Flush();

  SendPacket(features);
  LOG_INFO("[AUTH] SMSG_FEATURE_SYSTEM_STATUS sent (15595 parity)");
}

void WorldSession::SendRealmSplit(uint32 realmId) {
  WorldPacket split(SMSG_REALM_SPLIT);
  std::string splitDate = "01/01/01";

  split.Append<uint32>(realmId); // Realm ID
  split.Append<uint32>(0);       // State (0 = Normal)

  BitWriter bw(split);
  bw.WriteBits(static_cast<uint32>(splitDate.length()), 7);
  bw.Flush();

  split.WriteStringNoNull(splitDate);

  SendPacket(split);
  LOG_INFO("[AUTH] SMSG_REALM_SPLIT sent (Bit-packed 4.3.4)");
}

void WorldSession::SendSetTimeZoneInformation() {
  WorldPacket data(SMSG_SET_TIME_ZONE_INFORMATION);
  std::string serverTz = "UTC";
  std::string localTz = "UTC";

  BitWriter bw(data);
  bw.WriteBits(static_cast<uint32>(serverTz.length()), 7);
  bw.WriteBits(static_cast<uint32>(localTz.length()), 7);
  bw.Flush();

  data.WriteStringNoNull(serverTz);
  data.WriteStringNoNull(localTz);

  SendPacket(data);
  LOG_INFO("[AUTH] SMSG_SET_TIME_ZONE_INFORMATION sent (Bit-packed 4.3.4)");
}

void WorldSession::SendLearnedDanceMoves() {
  WorldPacket data(SMSG_LEARNED_DANCE_MOVES);
  BitWriter bw(data);
  bw.WriteBits(0, 23); // Count (23 bits in Cata 4.3.4)
  bw.Flush();
  SendPacket(data);
  LOG_INFO("[AUTH] SMSG_LEARNED_DANCE_MOVES sent (0 moves, bit-packed)");
}

void WorldSession::HandleReadyForAccountDataTimes(WorldPacket & /*packet*/) {
  LOG_INFO("CMSG_READY_FOR_ACCOUNT_DATA_TIMES received. Sending AccountDataTimes.");
  SendAccountDataTimes(0x15); // GLOBAL_CACHE_MASK
}

void WorldSession::HandleUpdateAccountData(WorldPacket &packet) {
  uint32 type = packet.Read<uint32>();
  uint32 time = packet.Read<uint32>();
  uint32 decompressedSize = packet.Read<uint32>();
  
  LOG_INFO("CMSG_UPDATE_ACCOUNT_DATA: Type {}, Time {}, Size {}. Absorbing to break sync loop.", 
           type, time, decompressedSize);
}

void WorldSession::SendLoginSetTimeSpeed() {
  WorldPacket speed(SMSG_LOGIN_SET_TIME_SPEED);
  speed.Append<uint32>(static_cast<uint32>(std::time(nullptr)));
  speed.Append<float>(0.01666667f); // Speed
  SendPacket(speed);
  LOG_INFO("[AUTH] SMSG_LOGIN_SET_TIME_SPEED sent");
}

void WorldSession::SendMotd() {
  WorldPacket motd(SMSG_MOTD);
  std::vector<std::string> lines = {"Welcome to Firelands WoW!"};

  motd.Append<uint32>(static_cast<uint32>(lines.size()));
  for (auto const &line : lines) {
    motd.WriteString(line); // Should be null-terminated
  }

  SendPacket(motd);
  LOG_INFO("[AUTH] SMSG_MOTD sent (4.3.4 byte-oriented format)");
}

void WorldSession::SendAccountRestrictedUpdate() {
  WorldPacket data(SMSG_ACCOUNT_RESTRICTED_UPDATE);
  BitWriter bw(data);
  bw.WriteBit(false); // isRestricted
  bw.Flush();
  SendPacket(data);
  LOG_INFO("[AUTH] SMSG_ACCOUNT_RESTRICTED_UPDATE sent");
}

void WorldSession::SendSetDfFastLaunchResources() {
  WorldPacket data(SMSG_SET_DF_FAST_LAUNCH_RESOURCES);
  BitWriter bw(data);
  bw.WriteBits(0, 32); // Count (32 bits for bits count?)
  bw.Flush();
  SendPacket(data);
  LOG_INFO("[AUTH] SMSG_SET_DF_FAST_LAUNCH_RESOURCES sent (Empty)");
}

void WorldSession::SendInitialRaidGroupError() {
  WorldPacket data(SMSG_INITIAL_RAID_GROUP_ERROR);
  SendPacket(data);
  LOG_INFO("[AUTH] SMSG_INITIAL_RAID_GROUP_ERROR sent");
}

void WorldSession::HandleCharEnum(WorldPacket & /*packet*/) {
  auto characters = _charService->GetCharactersForAccount(_accountId);
  uint32 count = static_cast<uint32>(characters.size());

  LOG_INFO("CMSG_CHAR_ENUM: Found {} characters for account {}", count, _accountId);

  WorldPacket response(SMSG_CHAR_ENUM);
  BitWriter bw(response);

  // Cata 4.3.4 (15595) Bit Header:
  bw.WriteBits(0, 23); // FactionChangeRestrictions size
  bw.WriteBit(true);   // Success
  bw.WriteBits(count, 17);

  if (count == 0) {
      SendPacket(response);
      return;
  }

  struct GuidData {
    uint8 g[8];
    uint8 gg[8];
  };
  std::vector<GuidData> gd_list(count);

  for (uint32 i = 0; i < count; ++i) {
    uint64 guid = characters[i]->GetGuid();
    uint64 guildGuid = 0; // Guilds not implemented
    
    for (int b = 0; b < 8; ++b) {
      gd_list[i].g[b] = (guid >> (b * 8)) & 0xFF;
      gd_list[i].gg[b] = (guildGuid >> (b * 8)) & 0xFF;
    }

    auto& gd = gd_list[i];
    // Exact bit order for 15595 Character Enum
    bw.WriteBit(gd.g[3]);
    bw.WriteBit(gd.gg[1]);
    bw.WriteBit(gd.gg[7]);
    bw.WriteBit(gd.gg[2]);
    bw.WriteBits(characters[i]->GetName().length(), 7);
    bw.WriteBit(gd.g[4]);
    bw.WriteBit(gd.g[7]);
    bw.WriteBit(gd.gg[3]);
    bw.WriteBit(gd.g[5]);
    bw.WriteBit(gd.gg[6]);
    bw.WriteBit(gd.g[1]);
    bw.WriteBit(gd.gg[5]);
    bw.WriteBit(gd.gg[4]);
    bw.WriteBit(characters[i]->IsFirstLogin());
    bw.WriteBit(gd.g[0]);
    bw.WriteBit(gd.g[2]);
    bw.WriteBit(gd.g[6]);
    bw.WriteBit(gd.gg[0]);
  }
  bw.Flush();

  for (uint32 i = 0; i < count; ++i) {
    auto const &ch = characters[i];
    auto &gd = gd_list[i];

    // Exact byte order for 15595 Character Enum
    response.Append<uint8>(ch->GetClass());

    // Equipment (VisualItems) - 23 slots in Cata
    // Order: InvType (uint8), DisplayID (uint32), DisplayEnchantID (uint32)
    for (int slot = 0; slot < 23; ++slot) {
        response.Append<uint8>(0);  // InvType
        response.Append<uint32>(0); // DisplayID
        response.Append<uint32>(0); // DisplayEnchantID
    }

    response.Append<uint32>(0); // PetCreatureFamilyID
    response.WriteByteSeq(gd.gg[2]);
    response.Append<uint8>(i); // ListPosition
    response.Append<uint8>(ch->GetHairStyle());
    response.WriteByteSeq(gd.gg[3]);
    response.Append<uint32>(0); // PetCreatureDisplayID
    response.Append<uint32>(ch->GetCharacterFlags());
    response.Append<uint8>(ch->GetHairColor());
    response.WriteByteSeq(gd.g[4]);
    response.Append<int32>(ch->GetMapId());
    response.WriteByteSeq(gd.gg[5]);
    response.Append<float>(ch->GetZ());
    response.WriteByteSeq(gd.gg[6]);
    response.Append<uint32>(0); // PetExperienceLevel
    response.WriteByteSeq(gd.g[3]);
    response.Append<float>(ch->GetY());
    response.Append<uint32>(ch->GetCustomizationFlags());
    response.Append<uint8>(ch->GetFacialHair());
    response.WriteByteSeq(gd.g[7]);
    response.Append<uint8>(ch->GetGender());
    response.WriteStringNoNull(ch->GetName());
    response.Append<uint8>(ch->GetFace());
    response.WriteByteSeq(gd.g[0]);
    response.WriteByteSeq(gd.g[2]);
    response.WriteByteSeq(gd.gg[1]);
    response.WriteByteSeq(gd.gg[7]);
    response.Append<float>(ch->GetX());
    response.Append<uint8>(ch->GetSkin());
    response.Append<uint8>(ch->GetRace());
    response.Append<uint8>(ch->GetLevel());
    response.WriteByteSeq(gd.g[6]);
    response.WriteByteSeq(gd.gg[4]);
    response.WriteByteSeq(gd.gg[0]);
    response.WriteByteSeq(gd.g[5]);
    response.WriteByteSeq(gd.g[1]);
    response.Append<int32>(ch->GetZoneId());
  }

  SendPacket(response);
  LOG_INFO("SMSG_CHAR_ENUM sent with {} characters (Full 15595 Parity)", count);
}

void WorldSession::HandleRealmSplit(WorldPacket &packet) {
  uint32 unk = packet.Read<uint32>();
  std::string splitDate = "01/01/01";

  WorldPacket data(SMSG_REALM_SPLIT);
  data.Append<uint32>(unk);
  data.Append<uint32>(0); // Normal state
  data.WriteString(splitDate);

  SendPacket(data);
  LOG_INFO("CMSG_REALM_SPLIT handled (Date: {})", splitDate);
}

void WorldSession::HandlePing(WorldPacket &packet) {
  uint32 latency = packet.Read<uint32>();
  uint32 serial = packet.Read<uint32>();
  LOG_INFO("CMSG_PING: latency={}, serial={}", latency, serial);

  WorldPacket pong(SMSG_PONG);
  pong.Append<uint32>(serial);
  SendPacket(pong);
}

void WorldSession::HandleCharCreate(WorldPacket &packet) {
  std::string name = packet.ReadString();
  uint8 race = packet.Read<uint8>();
  uint8 klass = packet.Read<uint8>();
  uint8 gender = packet.Read<uint8>();
  uint8 skin = packet.Read<uint8>();
  uint8 face = packet.Read<uint8>();
  uint8 hairStyle = packet.Read<uint8>();
  uint8 hairColor = packet.Read<uint8>();
  uint8 facialHair = packet.Read<uint8>();
  uint8 outfitId = packet.Read<uint8>();

  LOG_INFO("CMSG_CHAR_CREATE: Name='{}', Race={}, Class={}, Gender={}, Skin={}, Face={}, HairStyle={}, HairColor={}, FacialHair={}, Outfit={}", 
           name, race, klass, gender, skin, face, hairStyle, hairColor, facialHair, outfitId);

  bool success =
      _charService->CreateCharacter(_accountId, name, race, klass, gender, skin,
                                    face, hairStyle, hairColor, facialHair);

  WorldPacket response(SMSG_CHAR_CREATE);
  // 4.3.4 SMSG_CHAR_CREATE Response codes
  // CHAR_CREATE_SUCCESS = 0x2F
  // CHAR_CREATE_ERROR   = 0x30
  response.Append<uint8>(success ? 0x2F : 0x30); 

  SendPacket(response);
  LOG_INFO("SMSG_CHAR_CREATE sent result: {}", success ? "SUCCESS" : "FAIL");
}

void WorldSession::HandleCharDelete(WorldPacket &packet) {
  uint64 guid = packet.Read<uint64>();

  LOG_INFO("CMSG_CHAR_DELETE for GUID: {}", guid);

  bool success =
      _charService->DeleteCharacter(static_cast<uint32>(guid), _accountId);

  WorldPacket response(SMSG_CHAR_DELETE);
  // 4.3.4 SMSG_CHAR_DELETE Response
  // Success = 0x47, Error = 0x48 (Legacy but usually works)
  response.Append<uint8>(success ? 0x47 : 0x48);

  SendPacket(response);
  LOG_INFO("SMSG_CHAR_DELETE sent result: {}", success ? "SUCCESS" : "FAIL");
}

void WorldSession::HandlePlayerLogin(WorldPacket &packet) {
  uint8 guid_bytes[8] = {0};
  BitReader br(packet);
  
  // Cataclysm 4.3.4 (15595) CMSG_PLAYER_LOGIN GUID Bit Order
  br.ReadBit(); // Unk bit
  guid_bytes[1] = br.ReadBit() ? packet.Read<uint8>() : 0;
  guid_bytes[3] = br.ReadBit() ? packet.Read<uint8>() : 0;
  guid_bytes[5] = br.ReadBit() ? packet.Read<uint8>() : 0;
  guid_bytes[7] = br.ReadBit() ? packet.Read<uint8>() : 0;
  guid_bytes[2] = br.ReadBit() ? packet.Read<uint8>() : 0;
  guid_bytes[0] = br.ReadBit() ? packet.Read<uint8>() : 0;
  guid_bytes[4] = br.ReadBit() ? packet.Read<uint8>() : 0;
  guid_bytes[6] = br.ReadBit() ? packet.Read<uint8>() : 0;

  uint64 guid = 0;
  std::memcpy(&guid, guid_bytes, 8);

  LOG_INFO("CMSG_PLAYER_LOGIN for GUID: {}", guid);
  _playerGuid = guid;

  // 1. Send SMSG_LOGIN_VERIFY_WORLD
  WorldPacket verify(SMSG_LOGIN_VERIFY_WORLD);
  verify.Append<uint32>(0);   // Map ID (0 = Azeroth)
  verify.Append<float>(0.0f); // X
  verify.Append<float>(0.0f); // Y
  verify.Append<float>(0.0f); // Z
  verify.Append<float>(0.0f); // O
  SendPacket(verify);

  // 2. Send SMSG_FEATURE_SYSTEM_STATUS
  SendFeatureSystemStatus();

  // 3. Send SMSG_ACCOUNT_DATA_TIMES
  SendAccountDataTimes(0x15);

  // 4. Send MOTD
  SendMotd();

  // 5. Send SMSG_TUTORIAL_FLAGS (all zero)
  SendTutorialFlags();

  // 6. Send SMSG_INITIAL_SPELLS
  SendInitialSpells();

  // 7. Send SMSG_ACTION_BUTTONS
  SendInitialActionButtons();

  // 8. Send SMSG_TIME_SYNC_REQ
  WorldPacket timeSync(SMSG_TIME_SYNC_REQ);
  timeSync.Append<uint32>(0); // Counter
  SendPacket(timeSync);

  // 9. Send Set Time Speed
  SendLoginSetTimeSpeed();

  // 10. Send Initial Object Update (Spawn Player)
  SendInitialObjectUpdate(guid);

  LOG_INFO("Handshake for Player Login completed for GUID: {}", guid);
}

void WorldSession::SendInitialSpells() {
  WorldPacket data(SMSG_INITIAL_SPELLS);
  data.Append<uint8>(0);      // Gender (not used in packet?)
  data.Append<uint16>(0);     // Spell Count
  // If count > 0, append pairs of <uint32 spellId, uint16 unk>
  data.Append<uint16>(0);     // Cooldown Count
  SendPacket(data);
  LOG_INFO("[LOGIN] SMSG_INITIAL_SPELLS sent (Empty)");
}

void WorldSession::SendInitialActionButtons() {
  WorldPacket data(SMSG_ACTION_BUTTONS);
  data.Append<uint8>(0); // Clear all existing
  for (int i = 0; i < 144; ++i) {
      data.Append<uint32>(0); // Action
  }
  SendPacket(data);
  LOG_INFO("[LOGIN] SMSG_ACTION_BUTTONS sent (Empty, 144 slots)");
}

void WorldSession::SendInitialObjectUpdate(uint64 guid) {
  UpdateData update;

  MovementInfo move;
  move.time = static_cast<uint32>(std::time(nullptr));
  // In a real scenario, we would load X, Y, Z from the character object
  // For now, use 0,0,0 as in the previous implementation

  std::map<uint16, uint32> fields;
  fields[OBJECT_FIELD_GUID] = (uint32)(guid & 0xFFFFFFFF);
  fields[OBJECT_FIELD_GUID + 1] = (uint32)(guid >> 32);
  fields[OBJECT_FIELD_TYPE] =
      (1 << TYPEID_OBJECT) | (1 << TYPEID_UNIT) | (1 << TYPEID_PLAYER);
  fields[UNIT_FIELD_HEALTH] = 100;
  fields[UNIT_FIELD_MAXHEALTH] = 100;
  fields[UNIT_FIELD_LEVEL] = 1;

  update.AddCreateObject(guid, TYPEID_PLAYER, move, fields);

  WorldPacket packet;
  update.Build(packet);

  SendPacket(packet);
  LOG_INFO("SMSG_UPDATE_OBJECT sent (Player Spawned with basic fields using "
           "UpdateData)");
}

void WorldSession::HandleMessageChat(WorldPacket &packet) {
  uint32 type = packet.Read<uint32>();
  uint32 language = packet.Read<uint32>();

  std::string target;
  if (type == CHAT_MSG_WHISPER || type == CHAT_MSG_CHANNEL) {
    target = packet.ReadString();
  }

  std::string message = packet.ReadString();
  LOG_INFO("Chat from {}: [{}] {}", _playerGuid, type, message);

  // GM Command handling
  if (_commandService->IsCommand(message)) {
      _commandService->ExecuteCommand(shared_from_this(), message);
      return;
  }

  // Echo back to nearby players (spatial chat base)
  // For now, only send back to the sender
  WorldPacket response(SMSG_MESSAGECHAT);
  response.Append<uint8>(static_cast<uint8>(type));
  response.Append<uint32>(language);
  response.Append<uint64>(_playerGuid);
  response.Append<uint32>(0);           // Unk
  response.Append<uint64>(_playerGuid); // Target? or Sender? depends on type
  response.Append<uint32>(static_cast<uint32>(message.length() + 1));
  response.WriteString(message);
  response.Append<uint8>(0); // Tag

  SendPacket(response);
}

void WorldSession::HandleMovement(WorldPacket &packet) {
  MovementInfo move;
  ReadMovementInfo(packet, move);

  // Update internal state
  _position = move;

  LOG_TRACE("Player {} moved to ({:.2f}, {:.2f}, {:.2f}) O:{:.2f}", _playerGuid,
            move.x, move.y, move.z, move.orientation);

  // Broadcast movement (Echo back to client for now, or broadcast to others if
  // any) In WoW, we typically echo back the movement packet if it's an MSG
  // opcode
  if (packet.GetOpcode() >= 0x2000) { // Rough check for MSG opcodes if needed,
                                      // but we listed them explicitly
    // echo back
    WorldPacket echo(packet.GetOpcode(), packet.Size());
    echo.Append(packet.GetBuffer(), packet.Size());
    SendPacket(echo);
  }
}

void WorldSession::ReadMovementInfo(WorldPacket &packet, MovementInfo &move) {
  // Cataclysm 4.3.4: MovementInfo usually starts with the GUID
  // But some MSG opcodes have the GUID packed.
  // For simplicity, let's assume it matches our struct for now or handle the
  // common format

  // Skip GUID if present (it's usually 8 bytes or packed)
  // For build 15595, it's often PackedGuid.

  // We'll use a hack for now: try to read common field order
  // In a real emulator, we'd have a BitStream reader for the complex 4.x
  // packing

  // If the packet size is enough for our struct:
  if (packet.Size() >= 26) {
    // Basic structure: Flags(4), Flags2(2), Time(4), X, Y, Z, O (16) = 26 bytes
    move.flags = packet.Read<uint32>();
    move.flags2 = packet.Read<uint16>();
    move.time = packet.Read<uint32>();
    move.x = packet.Read<float>();
    move.y = packet.Read<float>();
    move.z = packet.Read<float>();
    move.orientation = packet.Read<float>();
  }
}

void WorldSession::SendNotification(const std::string& message) {
    WorldPacket response(SMSG_MESSAGECHAT);
    response.Append<uint8>(CHAT_MSG_SYSTEM);
    response.Append<uint32>(LANG_UNIVERSAL);
    response.Append<uint64>(0);           // Sender GUID
    response.Append<uint32>(0);           // Unk
    response.Append<uint64>(0);           // Target GUID
    response.Append<uint32>(static_cast<uint32>(message.length() + 1));
    response.WriteString(message);
    response.Append<uint8>(0);            // Tag
    SendPacket(response);
}

void WorldSession::TeleportTo(uint32 /*mapId*/, float x, float y, float z, float orientation) {
    _position.x = x;
    _position.y = y;
    _position.z = z;
    _position.orientation = orientation;
    
    // In Cata 4.3.4, a teleport requires SMSG_TRANSFER_PENDING then SMSG_NEW_WORLD if map changes
    // or SMSG_MONSTER_MOVE (for self) or UpdateObject for position reset.
    // For now, we just notify and update internal state for .gps
    SendNotification("Teleported to: " + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z));
}

} // namespace Firelands
