#include <infrastructure/network/sessions/WorldSession.h>
#include <shared/Logger.h>
#include <shared/Crypto.h>
#include <shared/network/UpdateFields.h>
#include <shared/network/UpdateData.h>
#include <domain/models/Chat.h>
#include <random>
#include <algorithm>
#include <ctime>

namespace Firelands {

    WorldSession::WorldSession(tcp::socket socket, std::shared_ptr<AuthService> authService, std::shared_ptr<CharacterService> charService)
        : _socket(std::move(socket)), _authService(std::move(authService)), _charService(std::move(charService)), _accountId(0) {
    }

    void WorldSession::Start() {
        LOG_INFO("WorldSession started for {}", GetIpAddress());
        
        // Cataclysm 4.3.4 Handshake: Server sends initializer string first (NO OPCODES)
        std::string initializer = "WORLD OF WARCRAFT CONNECTION - SERVER TO CLIENT";
        ByteBuffer buffer;
        
        // Header for the initializer: just [Size:2 (BE)], followed by the string payload.
        uint16 size = static_cast<uint16>(initializer.length());
        buffer.Append<uint8>((size >> 8) & 0xFF);
        buffer.Append<uint8>(size & 0xFF);
        buffer.Append((const uint8*)initializer.c_str(), initializer.length());
        
        SendPacket(buffer);
        DoRead();
    }

    void WorldSession::SendPacket(WorldPacket& packet) {
        ByteBuffer buffer;
        // In Cataclysm, Server header is [Size:2 (BE)][Opcode:2 (LE)]
        uint16 size = static_cast<uint16>(packet.Size() + 2);

        uint8 header[4];
        header[0] = (size >> 8) & 0xFF;
        header[1] = size & 0xFF;
        uint16 opcode = static_cast<uint16>(packet.GetOpcode());
        header[2] = opcode & 0xFF;
        header[3] = (opcode >> 8) & 0xFF;

        // Encrypt header if crypt is initialized (after CMSG_AUTH_SESSION)
        _crypt.EncryptSend(header, 4);

        buffer.Append(header, 4);
        buffer.Append(packet.GetBuffer(), packet.Size());

        SendPacket(buffer);
    }

    void WorldSession::SendPacket(ByteBuffer& buffer) {
        auto shared_buffer = std::make_shared<std::vector<uint8>>(buffer.GetBuffer(), buffer.GetBuffer() + buffer.Size());

        // Log hex of every outgoing packet for diagnostics
        {
            std::string hexDump;
            for (size_t i = 0; i < shared_buffer->size() && i < 64; ++i) {
                char hex[4];
                std::snprintf(hex, sizeof(hex), "%02X ", (*shared_buffer)[i]);
                hexDump += hex;
            }
            if (shared_buffer->size() > 64) hexDump += "...";
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

        boost::asio::async_write(_socket, boost::asio::buffer(buffer->data(), buffer->size()),
            [this, self, buffer](boost::system::error_code ec, std::size_t /*length*/) {
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
        WorldPacket packet(SMSG_AUTH_CHALLENGE);
        
        // TCPP 4.3.4 (15595) structure:
        // 1. DosChallenge (32 bytes - random)
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint32> dis(0, 0xFF);
        for (int i = 0; i < 32; ++i) packet.Append<uint8>(static_cast<uint8>(dis(gen)));
        
        // 2. Server Seed (4 bytes)
        packet.Append<uint32>(_serverSeed);

        // 3. DosZeroBits (1 byte flag)
        packet.Append<uint8>(1);
        
        SendPacket(packet);
        LOG_INFO("SMSG_AUTH_CHALLENGE sent (seed: 0x{:08X})", _serverSeed);
    }

    void WorldSession::Close() {
        _socket.close();
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
        _socket.async_read_some(boost::asio::buffer(_readBuffer, sizeof(_readBuffer)),
            [this, self](boost::system::error_code ec, std::size_t length) {
                if (!ec) {
                    // Log raw received data for debugging
                    LOG_INFO("[RECV] {} bytes: {}", length, 
                        Crypto::ToHexString(_readBuffer, std::min<size_t>(length, 32)));

                    _inBuffer.Append(_readBuffer, length);
                    
                    // Process as many packets as possible from the buffer
                    while (true) {
                        if (!_initialized) {
                            // Handshake: unencrypted [Size:2 BE][String]
                            if (_inBuffer.Size() < 2) break;
                            uint16 size = (_inBuffer[0] << 8) | _inBuffer[1];
                            if (_inBuffer.Size() < static_cast<size_t>(size + 2)) break;

                            ByteBuffer packetData;
                            packetData.Append(_inBuffer.GetBuffer(), size + 2);
                            HandlePacket(packetData);

                            std::vector<uint8> remaining(_inBuffer.GetBuffer() + size + 2, _inBuffer.GetBuffer() + _inBuffer.Size());
                            _inBuffer.Clear();
                            _inBuffer.Append(remaining.data(), remaining.size());
                            continue;
                        }

                        // Post-init: CMSG header = 6 bytes [Size:2 BE][Opcode:4 LE]
                        // These 6 bytes may be ARC4-encrypted.
                        if (_inBuffer.Size() < 6) break;

                        // Decrypt header exactly once per packet (ARC4 is stateful)
                        if (_crypt.IsInitialized() && !_headerDecrypted) {
                            std::memcpy(_decHeader, _inBuffer.GetBuffer(), 6);
                            _crypt.DecryptRecv(_decHeader, 6);
                            _headerDecrypted = true;
                        } else if (!_crypt.IsInitialized() && !_headerDecrypted) {
                            std::memcpy(_decHeader, _inBuffer.GetBuffer(), 6);
                            _headerDecrypted = true;
                        }

                        uint16 pktSize = (_decHeader[0] << 8) | _decHeader[1];

                        // Total on wire: 2 (size field) + pktSize
                        if (_inBuffer.Size() < static_cast<size_t>(pktSize + 2)) break;

                        _headerDecrypted = false;

                        uint32 opcode = _decHeader[2] | (_decHeader[3] << 8) | (_decHeader[4] << 16) | (_decHeader[5] << 24);
                        uint32 payloadSize = (pktSize >= 4) ? (pktSize - 4) : 0;

                        WorldPacket packet(opcode, payloadSize);
                        if (payloadSize > 0) {
                            packet.Append(_inBuffer.GetBuffer() + 6, std::min<size_t>(payloadSize, _inBuffer.Size() - 6));
                        }

                        // Remove consumed bytes
                        size_t consumed = pktSize + 2;
                        std::vector<uint8> remaining(_inBuffer.GetBuffer() + consumed, _inBuffer.GetBuffer() + _inBuffer.Size());
                        _inBuffer.Clear();
                        _inBuffer.Append(remaining.data(), remaining.size());

                        ProcessPacket(packet);
                    }
                    
                    DoRead();
                } else if (ec != boost::asio::error::operation_aborted) {
                    Close();
                }
            });
    }

    void WorldSession::HandlePacket(ByteBuffer& buffer) {
        // This method now ONLY handles the initial handshake.
        // Post-init packet processing with encryption is done in DoRead.
        std::string expected = "WORLD OF WARCRAFT CONNECTION - CLIENT TO SERVER";
        
        uint16 size = buffer.Read<uint16>();
        size = (size << 8) | (size >> 8);
        
        std::string received;
        for (uint16 i = 0; i < size; ++i) {
            received += static_cast<char>(buffer.Read<uint8>());
        }
        
        while (!received.empty() && (received.back() == '\0' || received.back() == '\r' || received.back() == '\n')) {
            received.pop_back();
        }
        
        if (received == expected) {
            LOG_INFO("WorldSession: Handshake string validated.");
            _initialized = true;
            
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<uint32> dis(0, 0xFFFFFFFF);
            _serverSeed = dis(gen);
            
            SendAuthChallenge();
        } else {
            LOG_ERROR("WorldSession: Invalid handshake string received. Expected '{}', got '{}'", expected, received);
            Close();
        }
    }

    void WorldSession::ProcessPacket(WorldPacket& packet) {
        uint32 opcode = packet.GetOpcode();
        LOG_INFO("WorldSession received packet: {}, size: {}", packet.GetOpcodeName(), packet.Size());

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
            case CMSG_LOG_DISCONNECT:
                LOG_INFO("Client disconnected (CMSG_LOG_DISCONNECT)");
                Close();
                break;
            default:
                LOG_WARN("[PACKET] Unknown/unhandled opcode: 0x{:04X} (size: {})", opcode, packet.Size());
                break;
        }
    }

    void WorldSession::HandleAuthSession(WorldPacket& packet) {
        // Cataclysm 4.3.4 (15595) can have different formats. 
        // Let's check the first 4 bytes to see if it's the build number (Standard format)
        uint32 buildCheck = 0;
        std::memcpy(&buildCheck, packet.GetBuffer(), 4);
        
        bool isScattered = (buildCheck != 15595);

        std::string account;
        uint16 build = 0;
        uint32 realmId = 0;
        uint8 digest[20];
        std::vector<uint8> localChallenge(4);

        if (isScattered) {
            int32 loginServerId = packet.Read<int32>();
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
            
            uint32 addonDataSize = packet.Read<uint32>();
            if (addonDataSize > 0) {
                packet.SetReadPos(packet.GetReadPos() + addonDataSize);
            }
        } else {
            // Standard format
            build = packet.Read<uint32>();
            uint32 unk = packet.Read<uint32>(); // loginServerId or unknown
            
            // Standard format often has strings at the end or bit-packed
            // But let's assume it has them as CStrings for now if it's a "clean" client
            // Actually, Cata clients almost always bit-pack strings.
        }
        
        // Bit-packed fields are always at the end in Cata
        uint32 bitPos = 8;
        uint8 curBitVal = 0;

        auto readBit = [&]() -> bool {
            if (bitPos >= 8) {
                curBitVal = packet.Read<uint8>();
                bitPos = 0;
            }
            return ((curBitVal >> (8 - ++bitPos)) & 1) != 0;
        };

        auto readBits = [&](int32 bits) -> uint32 {
            uint32 value = 0;
            for (int32 i = bits - 1; i >= 0; --i) {
                value |= static_cast<uint32>(readBit()) << i;
            }
            return value;
        };

        bool useIPv6 = readBit();
        uint16 nameLen = static_cast<uint16>(readBits(12));

        for (uint16 i = 0; i < nameLen; ++i) {
            account += static_cast<char>(packet.Read<uint8>());
        }

        LOG_INFO("CMSG_AUTH_SESSION: Account: '{}', Build: {}, RealmID: {}", account, build, realmId);

        
        // 1. Find account to get ID
        auto accountOpt = _authService->FindAccount(account);
        if (!accountOpt) {
            LOG_ERROR("CMSG_AUTH_SESSION: Account '{}' not found in database.", account);
            Close();
            return;
        }

        // 2. Get saved Session Key K from database
        std::vector<uint8_t> K = _authService->GetSessionKey(accountOpt->id);
        if (K.empty()) {
            LOG_ERROR("CMSG_AUTH_SESSION: No session key K found for account '{}'.", account);
            Close();
            return;
        }

        LOG_INFO("CMSG_AUTH_SESSION: Retrieved session key K for {}, length: {}", account, K.size());

        // 3. Perform Digest validation
        // Cata 4.3.4: Digest = SHA1(Account, LoginServerID, LocalChallenge, ServerSeed, SessionKey K)
        // Reference: AuthHandler.cpp in firelands-cata
        Crypto::SHA1 sha;
        sha.Update(Crypto::ToUpper(account));
        sha.Update<uint32>(0); // t = 0 in reference
        sha.Update(localChallenge);
        sha.Update<uint32>(_serverSeed);
        sha.Update(K);
        auto calculatedDigest = sha.Finalize();

        if (std::memcmp(calculatedDigest.data(), digest, 20) != 0) {
            LOG_ERROR("CMSG_AUTH_SESSION: Digest validation failed for account '{}'!", account);
            Close();
            return;
        }

        _accountId = accountOpt->id;
        LOG_INFO("CMSG_AUTH_SESSION: Digest validated successfully for account '{}' (ID: {}).", account, _accountId);

        // 4. Initialize ARC4 encryption FIRST, BEFORE sending AUTH_RESPONSE.
        // TCPP reference: "client cannot read auth response error without it"
        // The client expects ALL world packets (including AUTH_RESPONSE) to have encrypted headers.
        // 5. Send SMSG_AUTH_RESPONSE (Unencrypted - crypt starts AFTER this packet in Cata)
        // Cataclysm 4.3.4 bitfield format from TCPP reference:
        //   Result (u8)
        //   Bit: hasWaitInfo (false)
        //   Bit: hasSuccessInfo (true)
        //   If hasSuccessInfo: [Bit: isBattleNetAccount(false), Bit: isTwoFactorAbility(false)]
        //   FlushBits
        //   [SuccessInfo body: TimeRemain(u32), ActiveExpansion(u8), PCKickTime(u32), AccountExpansion(u8), TimeRested(u32), TimeOptions(u8)]
        {
            WorldPacket response(SMSG_AUTH_RESPONSE);
            response.Append<uint8>(0x0C); // Result: AUTH_OK

            BitWriter bw(response);
            bw.WriteBit(false);  // hasWaitInfo
            bw.WriteBit(true);   // hasSuccessInfo (AUTH_OK)
            bw.WriteBit(false);  // isBattleNetAccount
            bw.WriteBit(false);  // isTwoFactorAbility
            bw.Flush();

            response.Append<uint32>(0);    // TimeRemain
            response.Append<uint8>(3);     // ActiveExpansion (Cataclysm)
            response.Append<uint32>(0);    // KickTime
            response.Append<uint8>(3);     // AccountExpansion (Cataclysm)
            response.Append<uint32>(0);    // TimeRested
            response.Append<uint8>(0);     // TimeOptions

            SendPacket(response);
            LOG_INFO("[AUTH] SMSG_AUTH_RESPONSE sent (unencrypted transition packet)");
        }

        // Initialize encryption AFTER sending AUTH_RESPONSE
        _crypt.Init(K);
        LOG_INFO("[AUTH] WorldCrypt (ARC4) initialized for subsequent packets");

        // 6. Send SMSG_ADDON_INFO (Cataclysm 4.3.4 requires bit for addons)
        {
            WorldPacket addonInfo(SMSG_ADDON_INFO);
            addonInfo.Append<uint32>(0); // Banned addon count
            
            BitWriter bw(addonInfo);
            bw.WriteBit(false); // hasAddons
            bw.Flush();

            SendPacket(addonInfo);
            LOG_INFO("[AUTH] SMSG_ADDON_INFO sent (0 banned addons)");
        }

        // 7. Send SMSG_CLIENTCACHE_VERSION
        {
            WorldPacket cacheVer(SMSG_CLIENTCACHE_VERSION);
            cacheVer.Append<uint32>(0);
            SendPacket(cacheVer);
            LOG_INFO("[AUTH] SMSG_CLIENTCACHE_VERSION sent");
        }

        // 8. Send SMSG_TUTORIAL_FLAGS (8 x uint32 = 32 bytes)
        {
            WorldPacket tutorials(SMSG_TUTORIAL_FLAGS);
            for (int i = 0; i < 8; ++i) tutorials.Append<uint32>(0xFFFFFFFF);
            SendPacket(tutorials);
            LOG_INFO("[AUTH] SMSG_TUTORIAL_FLAGS sent");
        }

        // 9. Send SMSG_ACCOUNT_DATA_TIMES
        {
            WorldPacket data(SMSG_ACCOUNT_DATA_TIMES);
            data.Append<uint32>(static_cast<uint32>(std::time(nullptr))); 
            data.Append<uint8>(0);    // Flag (0)
            data.Append<uint32>(0);   // Mask (0)
            SendPacket(data);
            LOG_INFO("[AUTH] SMSG_ACCOUNT_DATA_TIMES sent");
        }

        // 10. Send SMSG_FEATURE_SYSTEM_STATUS (Cataclysm specific)
        {
            WorldPacket features(SMSG_FEATURE_SYSTEM_STATUS);
            features.Append<uint8>(1); // Feature count
            features.Append<uint8>(1); // Scroll of Resurrection
            features.Append<uint8>(0); // Status (disabled/off)
            SendPacket(features);
            LOG_INFO("[AUTH] SMSG_FEATURE_SYSTEM_STATUS sent");
        }

        // 11. Send SMSG_REALM_SPLIT (Known blocker for transition)
        {
            WorldPacket split(SMSG_REALM_SPLIT);
            split.Append<uint32>(0);          // Unknown
            split.Append<uint32>(1);          // State (1 = Normal)
            split.Append("01/01/01");         // Split date
            SendPacket(split);
            LOG_INFO("[AUTH] SMSG_REALM_SPLIT sent");
        }

        // 12. Send SMSG_LOGIN_SET_TIME_SPEED
        {
            WorldPacket speed(SMSG_LOGIN_SET_TIME_SPEED);
            speed.Append<uint32>(static_cast<uint32>(std::time(nullptr)));
            speed.Append<float>(0.01666667f); // Standard 1 min server time = 1 min real time
            speed.Append<uint32>(0);          // Unknown
            SendPacket(speed);
            LOG_INFO("[AUTH] SMSG_LOGIN_SET_TIME_SPEED sent");
        }

        // 13. Send SMSG_MOTD
        {
            WorldPacket motd(SMSG_MOTD);
            motd.Append<uint32>(1); // Line count
            motd.Append("Welcome to Firelands WoW!"); // Null-terminated string
            SendPacket(motd);
            LOG_INFO("[AUTH] SMSG_MOTD sent");
        }
    }

    void WorldSession::HandleCharEnum(WorldPacket& /*packet*/) {
        LOG_INFO("WorldSession::HandleCharEnum called for account ID: {}", _accountId);
        auto characters = _charService->GetCharactersForAccount(_accountId);
        uint32 count = static_cast<uint32>(characters.size());

        LOG_INFO("CMSG_CHAR_ENUM: Found {} characters for account {}", count, _accountId);

        struct GuidData {
            uint8 g[8];
            uint8 gg[8];
        };
        std::vector<GuidData> guidData(count);
        for (uint32 i = 0; i < count; ++i) {
            uint64 guid = characters[i]->GetGuid();
            uint64 guildGuid = 0; // Guilds not implemented yet

            for (int b = 0; b < 8; ++b) {
                guidData[i].g[b] = static_cast<uint8>((guid >> (b * 8)) & 0xFF);
                guidData[i].gg[b] = static_cast<uint8>((guildGuid >> (b * 8)) & 0xFF);
            }
        }

        WorldPacket response(SMSG_CHAR_ENUM);
        BitWriter bw(response);

        // Cata 4.3.4 Bit Header for SMSG_CHAR_ENUM:
        // 23 bits: FactionChangeRestrictions.size()
        // 1 bit: Success (true)
        // 17 bits: Characters.size()
        bw.WriteBits(0, 23); // FactionChangeRestrictions size
        bw.WriteBit(true);   // Success
        bw.WriteBits(count, 17);

        for (uint32 i = 0; i < count; ++i) {
            auto const& ch = characters[i];
            auto& gd = guidData[i];

            // Scrambled GUID bits (Exact order from TCPP CharacterPackets.cpp)
            bw.WriteBit(gd.g[3] != 0);
            bw.WriteBit(gd.gg[1] != 0);
            bw.WriteBit(gd.gg[7] != 0);
            bw.WriteBit(gd.gg[2] != 0);
            bw.WriteBits(static_cast<uint32>(ch->GetName().length()), 7);
            bw.WriteBit(gd.g[4] != 0);
            bw.WriteBit(gd.g[7] != 0);
            bw.WriteBit(gd.gg[3] != 0);
            bw.WriteBit(gd.g[5] != 0);
            bw.WriteBit(gd.gg[6] != 0);
            bw.WriteBit(gd.g[1] != 0);
            bw.WriteBit(gd.gg[5] != 0);
            bw.WriteBit(gd.gg[4] != 0);
            bw.WriteBit(ch->IsFirstLogin());
            bw.WriteBit(gd.g[0] != 0);
            bw.WriteBit(gd.g[2] != 0);
            bw.WriteBit(gd.g[6] != 0);
            bw.WriteBit(gd.gg[0] != 0);
        }

        bw.Flush();

        // Phase 3: Per-character byte data (exact field order from TCPP)
        for (uint32 i = 0; i < count; ++i) {
            auto const& ch = characters[i];
            auto& gd = guidData[i];

            response.Append<uint8>(ch->GetClass());

            // Equipment: 23 visual item slots
            for (int slot = 0; slot < 23; ++slot) {
                response.Append<uint8>(0);   // InvType
                response.Append<uint32>(0);  // DisplayID
                response.Append<uint32>(0);  // DisplayEnchantID
            }

            response.Append<uint32>(0);            // PetCreatureFamilyID
            response.WriteByteSeq(gd.gg[2]);       // GuildGUID[2]
            response.Append<uint8>(0);             // ListPosition
            response.Append<uint8>(ch->GetHairStyle());
            response.WriteByteSeq(gd.gg[3]);       // GuildGUID[3]
            response.Append<uint32>(0);            // PetCreatureDisplayID
            response.Append<uint32>(ch->GetCharacterFlags());
            response.Append<uint8>(ch->GetHairColor());
            response.WriteByteSeq(gd.g[4]);        // Guid[4]
            response.Append<int32>(ch->GetMapId());
            response.WriteByteSeq(gd.gg[5]);       // GuildGUID[5]
            response.Append<float>(ch->GetZ());
            response.WriteByteSeq(gd.gg[6]);       // GuildGUID[6]
            response.Append<uint32>(0);            // PetExperienceLevel
            response.WriteByteSeq(gd.g[3]);        // Guid[3]
            response.Append<float>(ch->GetY());
            response.Append<uint32>(ch->GetCustomizationFlags());
            response.Append<uint8>(ch->GetFacialHair());
            response.WriteByteSeq(gd.g[7]);        // Guid[7]
            response.Append<uint8>(ch->GetGender());
            response.WriteStringNoNull(ch->GetName());
            response.Append<uint8>(ch->GetFace());
            response.WriteByteSeq(gd.g[0]);        // Guid[0]
            response.WriteByteSeq(gd.g[2]);        // Guid[2]
            response.WriteByteSeq(gd.gg[1]);       // GuildGUID[1]
            response.WriteByteSeq(gd.gg[7]);       // GuildGUID[7]
            response.Append<float>(ch->GetX());
            response.Append<uint8>(ch->GetSkin());
            response.Append<uint8>(ch->GetRace());
            response.Append<uint8>(ch->GetLevel());
            response.WriteByteSeq(gd.g[6]);        // Guid[6]
            response.WriteByteSeq(gd.gg[4]);       // GuildGUID[4]
            response.WriteByteSeq(gd.gg[0]);       // GuildGUID[0]
            response.WriteByteSeq(gd.g[5]);        // Guid[5]
            response.WriteByteSeq(gd.g[1]);        // Guid[1]
            response.Append<int32>(ch->GetZoneId());
        }
        
        SendPacket(response);
        LOG_INFO("SMSG_CHAR_ENUM sent with {} characters (TCPP format)", count);
    }

    void WorldSession::HandlePing(WorldPacket& packet) {
        uint32 latency = packet.Read<uint32>();
        uint32 serial = packet.Read<uint32>();
        LOG_INFO("CMSG_PING: latency={}, serial={}", latency, serial);

        WorldPacket pong(SMSG_PONG);
        pong.Append<uint32>(serial);
        SendPacket(pong);
    }

    void WorldSession::HandleCharCreate(WorldPacket& packet) {
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

        LOG_INFO("CMSG_CHAR_CREATE for '{}' (Race: {}, Class: {})", name, race, klass);

        bool success = _charService->CreateCharacter(_accountId, name, race, klass, gender, skin, face, hairStyle, hairColor, facialHair);

        WorldPacket response(SMSG_CHAR_CREATE);
        // 4.3.4 SMSG_CHAR_CREATE Response
        response.Append<uint8>(success ? 0x31 : 0x32); // CHAR_CREATE_SUCCESS=0x31, CHAR_CREATE_ERROR=0x32 (Cata 4.3.4)

        SendPacket(response);
        LOG_INFO("SMSG_CHAR_CREATE sent result: {}", success ? "SUCCESS" : "FAIL");
    }

    void WorldSession::HandleCharDelete(WorldPacket& packet) {
        uint64 guid = packet.Read<uint64>();

        LOG_INFO("CMSG_CHAR_DELETE for GUID: {}", guid);

        bool success = _charService->DeleteCharacter(static_cast<uint32>(guid), _accountId);

        WorldPacket response(SMSG_CHAR_DELETE);
        // 4.3.4 SMSG_CHAR_DELETE Response
        // Success = 0x47, Error = 0x48 (Legacy but usually works)
        response.Append<uint8>(success ? 0x47 : 0x48); 

        SendPacket(response);
        LOG_INFO("SMSG_CHAR_DELETE sent result: {}", success ? "SUCCESS" : "FAIL");
    }

    void WorldSession::HandlePlayerLogin(WorldPacket& packet) {
        uint64 guid = packet.Read<uint64>();
        LOG_INFO("CMSG_PLAYER_LOGIN for GUID: {}", guid);
        _playerGuid = guid;

        // 1. Send SMSG_LOGIN_VERIFY_WORLD
        WorldPacket verify(SMSG_LOGIN_VERIFY_WORLD);
        verify.Append<uint32>(0);      // Map ID (0 = Azeroth)
        verify.Append<float>(0.0f);    // X
        verify.Append<float>(0.0f);    // Y
        verify.Append<float>(0.0f);    // Z
        verify.Append<float>(0.0f);    // O
        SendPacket(verify);

        // 2. Send SMSG_TUTORIAL_FLAGS (all zero)
        WorldPacket tutorials(SMSG_TUTORIAL_FLAGS);
        for (int i = 0; i < 8; ++i) tutorials.Append<uint32>(0);
        SendPacket(tutorials);

        // 3. Send SMSG_TIME_SYNC_REQ
        WorldPacket timeSync(SMSG_TIME_SYNC_REQ);
        timeSync.Append<uint32>(0); // Counter
        SendPacket(timeSync);

        SendInitialObjectUpdate(guid);

        // 4. Send SMSG_ACCOUNT_DATA_TIMES
        WorldPacket accountData(SMSG_ACCOUNT_DATA_TIMES);
        accountData.Append<uint32>(time(nullptr)); 
        accountData.Append<uint8>(1); // Unk
        accountData.Append<uint32>(0); // Mask
        SendPacket(accountData);

        // 5. Send SMSG_MOTD
        WorldPacket motd(SMSG_MOTD);
        motd.Append<uint32>(1); // Line count
        motd.Append("Welcome to Firelands Emulator (Cataclysm 4.3.4)");
        SendPacket(motd);

        LOG_INFO("Handshake for Player Login completed for GUID: {}", guid);
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
        fields[OBJECT_FIELD_TYPE] = (1 << TYPEID_OBJECT) | (1 << TYPEID_UNIT) | (1 << TYPEID_PLAYER);
        fields[UNIT_FIELD_HEALTH] = 100;
        fields[UNIT_FIELD_MAXHEALTH] = 100;
        fields[UNIT_FIELD_LEVEL] = 1;

        update.AddCreateObject(guid, TYPEID_PLAYER, move, fields);
        
        WorldPacket packet;
        update.Build(packet);
        
        SendPacket(packet);
        LOG_INFO("SMSG_UPDATE_OBJECT sent (Player Spawned with basic fields using UpdateData)");
    }

    void WorldSession::HandleMessageChat(WorldPacket& packet) {
        uint32 type = packet.Read<uint32>();
        uint32 language = packet.Read<uint32>();
        
        std::string target;
        if (type == CHAT_MSG_WHISPER || type == CHAT_MSG_CHANNEL) {
            target = packet.ReadString();
        }
        
        std::string message = packet.ReadString();
        LOG_INFO("Chat from {}: [{}] {}", _playerGuid, type, message);
        
        // Echo back to nearby players (spatial chat base)
        // For now, only send back to the sender
        WorldPacket response(SMSG_MESSAGECHAT);
        response.Append<uint8>(static_cast<uint8>(type));
        response.Append<uint32>(language);
        response.Append<uint64>(_playerGuid);
        response.Append<uint32>(0); // Unk
        response.Append<uint64>(_playerGuid); // Target? or Sender? depends on type
        response.Append<uint32>(static_cast<uint32>(message.length() + 1));
        response.WriteString(message);
        response.Append<uint8>(0); // Tag
        
        SendPacket(response);
    }

    void WorldSession::HandleMovement(WorldPacket& packet) {
        MovementInfo move;
        ReadMovementInfo(packet, move);
        
        // Update internal state
        _position = move;
        
        LOG_TRACE("Player {} moved to ({:.2f}, {:.2f}, {:.2f}) O:{:.2f}", _playerGuid, move.x, move.y, move.z, move.orientation);
        
        // Broadcast movement (Echo back to client for now, or broadcast to others if any)
        // In WoW, we typically echo back the movement packet if it's an MSG opcode
        if (packet.GetOpcode() >= 0x2000) { // Rough check for MSG opcodes if needed, but we listed them explicitly
             // echo back
             WorldPacket echo(packet.GetOpcode(), packet.Size());
             echo.Append(packet.GetBuffer(), packet.Size());
             SendPacket(echo);
        }
    }

    void WorldSession::ReadMovementInfo(WorldPacket& packet, MovementInfo& move) {
        // Cataclysm 4.3.4: MovementInfo usually starts with the GUID
        // But some MSG opcodes have the GUID packed.
        // For simplicity, let's assume it matches our struct for now or handle the common format
        
        // Skip GUID if present (it's usually 8 bytes or packed)
        // For build 15595, it's often PackedGuid.
        
        // We'll use a hack for now: try to read common field order
        // In a real emulator, we'd have a BitStream reader for the complex 4.x packing
        
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

} // namespace Firelands
