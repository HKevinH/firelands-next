#include <infrastructure/network/sessions/WorldSession.h>
#include <shared/Logger.h>
#include <shared/Crypto.h>
#include <random>
#include <algorithm>

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
        buffer.Append<uint8>((size >> 8) & 0xFF);
        buffer.Append<uint8>(size & 0xFF);
        
        uint16 opcode = static_cast<uint16>(packet.GetOpcode());
        buffer.Append<uint8>(opcode & 0xFF);
        buffer.Append<uint8>((opcode >> 8) & 0xFF);
        
        buffer.Append(packet.GetBuffer(), packet.Size());

        SendPacket(buffer);
    }

    void WorldSession::SendPacket(ByteBuffer& buffer) {
        auto self(shared_from_this());
        
        // We MUST capture the buffer data by shared_ptr to keep it alive 
        // during the asynchronous write operation.
        auto shared_buffer = std::make_shared<std::vector<uint8>>(buffer.GetBuffer(), buffer.GetBuffer() + buffer.Size());
        
        boost::asio::async_write(_socket, boost::asio::buffer(shared_buffer->data(), shared_buffer->size()),
            [this, self, shared_buffer](boost::system::error_code ec, std::size_t /*length*/) {
                if (ec) {
                    Close();
                }
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
                    _inBuffer.Append(_readBuffer, length);
                    
                    // Process as many packets as possible from the buffer
                    while (_inBuffer.Size() >= 6) {
                        // Read size (BE)
                        uint16 size = (_inBuffer[0] << 8) | _inBuffer[1];
                        
                        // Total packet length: 2 bytes size + payload (which includes 4 bytes opcode)
                        // Wait, size is payload_length + 4.
                        // Total on-wire: size + 2.
                        if (_inBuffer.Size() < static_cast<size_t>(size + 2)) {
                            break; // Wait for more data
                        }
                        
                        // Extract one packet
                        ByteBuffer packetData;
                        packetData.Append(_inBuffer.GetBuffer(), size + 2);
                        HandlePacket(packetData);
                        
                        // Remove processed packet from buffer
                        std::vector<uint8> remaining(_inBuffer.GetBuffer() + size + 2, _inBuffer.GetBuffer() + _inBuffer.Size());
                        _inBuffer.Clear();
                        _inBuffer.Append(remaining.data(), remaining.size());
                    }
                    
                    DoRead();
                } else if (ec != boost::asio::error::operation_aborted) {
                    Close();
                }
            });
    }

    void WorldSession::HandlePacket(ByteBuffer& buffer) {
        if (!_initialized) {
            std::string expected = "WORLD OF WARCRAFT CONNECTION - CLIENT TO SERVER";
            
            // Expected packet from client is [Size:2 (BE)] followed by the string.
            // So if buffer Size is smaller than expected string size + 2, we should wait.
            // But we already know buffer has at least 6 bytes from DoRead logic.
            // Let's just read the size.
            uint16 size = buffer.Read<uint16>();
            size = (size << 8) | (size >> 8);
            
            // Read exactly 'size' bytes for the string
            std::string received;
            for (uint16 i = 0; i < size; ++i) {
                // ByteBuffer::Read bounds-checks internally
                received += static_cast<char>(buffer.Read<uint8>());
            }
            
            // Clean up trailing null bytes or newlines that might cause false negatives
            while (!received.empty() && (received.back() == '\0' || received.back() == '\r' || received.back() == '\n')) {
                received.pop_back();
            }
            
            if (received == expected) {
                LOG_INFO("WorldSession: Handshake string validated.");
                _initialized = true;
                
                // Now that we are initialized, send the Auth Challenge
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<uint32> dis(0, 0xFFFFFFFF);
                _serverSeed = dis(gen);
                
                SendAuthChallenge();
            } else {
                LOG_ERROR("WorldSession: Invalid handshake string received. Expected '{}', got '{}'", expected, received);
                Close();
            }
            return;
        }

        if (buffer.Size() < 6) return;

        // In WoW 4.3.4, Client header is [Size:2 (BE)][Opcode:4 (LE)]
        uint16 size = buffer.Read<uint16>();
        size = (size << 8) | (size >> 8); 

        uint32 opcode = buffer.Read<uint32>();
        
        // In 4.3.4, the size field includes the 4 bytes of the opcode.
        // So payload size is size - 4.
        uint32 payloadSize = (size >= 4) ? (size - 4) : 0;
        
        WorldPacket packet(opcode, payloadSize);
        if (payloadSize > 0) {
            // Read exactly payloadSize bytes from the buffer starting at current read position (6)
            packet.Append(buffer.GetBuffer() + 6, std::min<size_t>(payloadSize, buffer.Size() - 6));
        }

        ProcessPacket(packet);
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
            case CMSG_LOG_DISCONNECT:
                LOG_INFO("Client disconnected (CMSG_LOG_DISCONNECT)");
                Close();
                break;
            default:
                LOG_WARN("Unknown or unhandled world opcode: 0x{:04X}", opcode);
                break;
        }
    }

    void WorldSession::HandleAuthSession(WorldPacket& packet) {
        // Cata 4.3.4 CMSG_AUTH_SESSION is extremely scattered.
        // We must follow the order in WorldPackets::Auth::AuthSession::Read() exactly.
        
        int32 loginServerId = packet.Read<int32>();
        uint32 battlegroupId = packet.Read<uint32>();
        int8 loginServerType = packet.Read<int8>();
        
        uint8 digest[20];
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
        
        uint16 build = packet.Read<uint16>();
        digest[8] = packet.Read<uint8>();
        
        uint32 realmId = packet.Read<uint32>();
        int8 buildType = packet.Read<int8>();
        
        digest[17] = packet.Read<uint8>();
        digest[6] = packet.Read<uint8>();
        digest[0] = packet.Read<uint8>();
        digest[1] = packet.Read<uint8>();
        digest[11] = packet.Read<uint8>();
        
        std::vector<uint8> localChallenge(4);
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
            // Skip addon data for now
            for (uint32 i = 0; i < addonDataSize; ++i) packet.Read<uint8>();
        }
        
        // Bit-packed fields at the end
        // UseIPv6 (1), AccountNameLength (12), AccountName (String)
        
        // Logic for bit reading (matching TrinityCore's MSB-first logic)
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

        std::string account;
        for (uint16 i = 0; i < nameLen; ++i) {
            account += static_cast<char>(packet.Read<uint8>());
        }

        LOG_INFO("CMSG_AUTH_SESSION: Account: {}, Build: {}, RealmID: {}", account, build, realmId);
        
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

        // 4. Send SMSG_AUTH_RESPONSE
        WorldPacket response(SMSG_AUTH_RESPONSE);
        // In 4.3.4 SMSG_AUTH_RESPONSE: [Bit: HasWaitInfo][Bit: HasSuccessInfo][Flush][WaitData...][SuccessData...]
        // [uint8 Result]
        
        BitWriter bw(response);
        bw.WriteBit(false); // HasWaitInfo
        bw.WriteBit(true);  // HasSuccessInfo
        bw.Flush();
        
        // SuccessInfo Data
        response.Append<uint32>(0);    // TimeRemain
        response.Append<uint8>(3);     // ActiveExpansion (3 = Cataclysm)
        response.Append<uint32>(0);    // TimeSecondsUntilPCKick
        response.Append<uint8>(3);     // AccountExpansion (3 = Cataclysm)
        response.Append<uint32>(0);    // TimeRested
        response.Append<uint8>(0);     // TimeOptions (0 = None)
        
        response.Append<uint8>(0x0C); // Result (0x0C = AUTH_OK)

        SendPacket(response);
        LOG_INFO("SMSG_AUTH_RESPONSE sent: AUTH_OK");
    }

    void WorldSession::HandleCharEnum(WorldPacket& /*packet*/) {
        LOG_INFO("WorldSession::HandleCharEnum called for account ID: {}", _accountId);
        auto characters = _charService->GetCharactersForAccount(_accountId);

        WorldPacket response(SMSG_CHAR_ENUM);
        // In 4.3.4 SMSG_CHAR_ENUM is bit-packed.
        // Extremely simplified layout for 0 characters (common case for fresh accounts):
        // [1 bit: Unk][21 bits: Count][... rest of characters]
        
        uint32 count = static_cast<uint32>(characters.size());
        
        // I'll implement a proper (but minimal) BitStream logic here for the count.
        // Count is 21 bits.
        uint32 packedCount = count; 
        response.Append<uint8>(0); // Unk bit
        response.Append<uint8>(packedCount & 0xFF);
        response.Append<uint8>((packedCount >> 8) & 0xFF);
        response.Append<uint8>((packedCount >> 16) & 0xFF);
        
        // For each character, there's a lot of data.
        // If count > 0, we'd append them here.
        
        SendPacket(response);
        LOG_INFO("SMSG_CHAR_ENUM sent with {} characters", count);
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

    void WorldSession::WritePackedGuid(uint64 guid, BitWriter& bw, ByteBuffer& bb) {
        // Simple bitmask-led packing for 4.x
        uint8 mask = 0;
        uint8 bytes[8];
        uint8 count = 0;
        
        for (int i = 0; i < 8; ++i) {
            uint8 b = (guid >> (i * 8)) & 0xFF;
            if (b) {
                mask |= (1 << i);
                bytes[count++] = b;
            }
        }
        
        bb.Append<uint8>(mask);
        for (int i = 0; i < count; ++i) {
            bb.Append<uint8>(bytes[i]);
        }
    }

    void WorldSession::SendInitialObjectUpdate(uint64 guid) {
        WorldPacket packet(SMSG_UPDATE_OBJECT);
        packet.Append<uint32>(1); // Number of updates
        packet.Append<uint8>(2);  // UPDATETYPE_CREATE_OBJECT (Active Player)

        BitWriter bw(packet);
        WritePackedGuid(guid, bw, packet);
        
        packet.Append<uint8>(4); // TYPEID_PLAYER

        // Movement Data Bits
        bw.WriteBit(true);  // Has movement data
        bw.WriteBit(false); // No transport
        bw.WriteBit(true);  // Is Self
        bw.WriteBit(false); // No stationary
        bw.WriteBit(false); // No animkits
        bw.Flush();

        // Movement Data Bytes
        packet.Append<uint32>(0);    // Movement Flags
        packet.Append<uint16>(0);    // Movement Flags 2
        packet.Append<uint32>(0);    // Time
        packet.Append<float>(0.0f);  // X
        packet.Append<float>(0.0f);  // Y
        packet.Append<float>(0.0f);  // Z
        packet.Append<float>(0.0f);  // Orientation
        packet.Append<uint32>(0);    // Fall time

        // Other bit-packed data (simplified)
        bw.WriteBit(false); // No victim
        bw.WriteBit(false); // No vehicle
        bw.WriteBit(false); // No pvp guid
        bw.Flush();

        // Update Fields (Zero Mask = No fields sent)
        packet.Append<uint32>(0); 

        SendPacket(packet);
        LOG_INFO("SMSG_UPDATE_OBJECT sent (Player Spawned)");
    }

} // namespace Firelands
