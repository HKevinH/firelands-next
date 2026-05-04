#include <application/services/OnlineCharacterSessionRegistry.h>
#include <infrastructure/network/sessions/WorldSession.h>
#include <shared/Logger.h>
#include <shared/network/WorldOpcodes.h>
#include <shared/network/WorldPacket.h>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

namespace Firelands {

void WorldSession::Start() {
  LOG_DEBUG("WorldSession started for {}", GetIpAddress());

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
  _lastSentOpcode = packet.GetOpcode();
  _lastSentPayloadSize = static_cast<uint32>(packet.Size());

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

  LOG_DEBUG("[SMSG] {} payload={} wire={}",
            packet.GetOpcodeName(), packet.Size(), buffer.Size());

  auto shared_buffer = std::make_shared<std::vector<uint8>>(
      buffer.GetBuffer(), buffer.GetBuffer() + buffer.Size());
  QueueOutgoing(shared_buffer);
}

void WorldSession::SendPacket(ServerPacket *packet) {
  if (packet) {
    SendPacket(*const_cast<WorldPacket *>(packet->Write()));
    delete packet;
  }
}

void WorldSession::QueueOutgoing(std::shared_ptr<std::vector<uint8>> buffer) {
  _writeQueue.push_back(std::move(buffer));
  if (!_writing) {
    DoWrite();
  }
}

void WorldSession::SendPacket(ByteBuffer &buffer) {
  auto shared_buffer = std::make_shared<std::vector<uint8>>(
      buffer.GetBuffer(), buffer.GetBuffer() + buffer.Size());
  LOG_DEBUG("[SEND] raw {} bytes (handshake / non-opcode)", shared_buffer->size());
  QueueOutgoing(shared_buffer);
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

// Cataclysm 4.3.4 (15595) SMSG_AUTH_CHALLENGE (37 bytes):
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
}

void WorldSession::UnregisterFromOnlineCharacterRegistryIfNeeded() {
  if (!_onlineCharRegistry || _activeCharacterName.empty())
    return;
  _onlineCharRegistry->Unregister(_activeCharacterName, _playerGuid, this);
  _activeCharacterName.clear();
}

void WorldSession::Close() {
  if (_playerGuid != 0) {
    LOG_INFO("Session disconnect: Account={} IP={} Character={} (saving and removing from world)",
             _accountId, GetIpAddress(), _playerGuid);
    FinalizeWorldExit();
    LOG_DEBUG("Character removed from world: Account={} GUID={}", _accountId,
              _playerGuid);
  } else {
    UnregisterFromOnlineCharacterRegistryIfNeeded();
  }
  CancelPeriodicTimeSync();
  if (_socket.is_open()) {
    LOG_DEBUG("Closing socket for Account={} IP={}", _accountId, GetIpAddress());
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

            // Total on wire: 2 (size field) + pktSize
            if (_inBuffer.Size() < static_cast<size_t>(pktSize + 2)) {
              if (_inBuffer.Size() >= 6) {
                LOG_DEBUG("Waiting for more data. Have {}, need {}",
                          _inBuffer.Size(), pktSize + 2);
              }
              break;
            }

            _headerDecrypted = false;

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
          if (_lastSentOpcode) {
            LOG_ERROR("Last SMSG before disconnect: opcode=0x{:04X} payload={}",
                      _lastSentOpcode, _lastSentPayloadSize);
          }
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
    LOG_DEBUG("WorldSession: Handshake string validated.");
    _initialized = true;

    SendAuthChallenge();
  } else {
    LOG_ERROR("WorldSession: Invalid handshake string received. Expected '{}', "
              "got '{}'",
              expected, received);
    Close();
  }
}

} // namespace Firelands
