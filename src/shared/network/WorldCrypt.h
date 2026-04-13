#pragma once

#include <shared/Common.h>
#include <cstring>
#include <vector>
#include <openssl/hmac.h>

namespace Firelands {

    /**
     * @brief Minimal ARC4 (Alleged RC4) stream cipher implementation.
     * Used for encrypting/decrypting WoW packet headers after authentication.
     */
    class ARC4 {
    public:
        void Init(const uint8* key, size_t keyLen) {
            for (int i = 0; i < 256; ++i) _s[i] = static_cast<uint8>(i);
            uint8 j = 0;
            for (int i = 0; i < 256; ++i) {
                j = j + _s[i] + key[i % keyLen];
                std::swap(_s[i], _s[j]);
            }
            _i = _j = 0;
        }

        void Process(uint8* data, size_t len) {
            for (size_t k = 0; k < len; ++k) {
                _i = (_i + 1);
                _j = (_j + _s[_i]);
                std::swap(_s[_i], _s[_j]);
                data[k] ^= _s[static_cast<uint8>(_s[_i] + _s[_j])];
            }
        }

    private:
        uint8 _s[256]{};
        uint8 _i = 0, _j = 0;
    };

    /**
     * @brief Handles ARC4 encryption/decryption of WoW packet headers.
     * 
     * In Cataclysm 4.3.4, after CMSG_AUTH_SESSION is processed:
     * - SMSG headers (4 bytes: Size[2] + Opcode[2]) are encrypted by the server.
     * - CMSG headers (6 bytes: Size[2] + Opcode[4]) are encrypted by the client.
     * - Packet BODIES are NOT encrypted.
     * 
     * Two ARC4 ciphers are used (one per direction), keyed with HMAC-SHA1(SessionKey, Seed).
     * The first 1024 bytes of each ARC4 keystream are dropped.
     */
    class WorldCrypt {
    public:
        void Init(const std::vector<uint8_t>& sessionKey) {
            // Well-known seeds shared between all Cataclysm 4.x clients and servers
            static const uint8_t kServerEncryptSeed[] = {
                0xCC, 0x98, 0xAE, 0x04, 0xE8, 0x97, 0xEA, 0xCA,
                0x12, 0xDD, 0xC0, 0x93, 0x42, 0x91, 0x53, 0x57
            };
            static const uint8_t kClientDecryptSeed[] = {
                0xC2, 0xB3, 0x72, 0x3C, 0xC6, 0xAE, 0xD9, 0xB5,
                0x34, 0x3C, 0x53, 0xEE, 0x2F, 0x43, 0x67, 0xCE
            };

            // Derive per-direction keys: HMAC-SHA1(SessionKey, Seed) → 20-byte key
            uint8_t serverKey[20], clientKey[20];
            unsigned int len = 20;

            HMAC(EVP_sha1(), sessionKey.data(), static_cast<int>(sessionKey.size()),
                 kServerEncryptSeed, sizeof(kServerEncryptSeed), serverKey, &len);
            HMAC(EVP_sha1(), sessionKey.data(), static_cast<int>(sessionKey.size()),
                 kClientDecryptSeed, sizeof(kClientDecryptSeed), clientKey, &len);

            _encrypt.Init(serverKey, 20);
            _decrypt.Init(clientKey, 20);

            // Drop first 1024 bytes of each keystream (standard WoW procedure)
            uint8_t drop[1024];
            std::memset(drop, 0, sizeof(drop));
            _encrypt.Process(drop, sizeof(drop));
            _decrypt.Process(drop, sizeof(drop));

            _initialized = true;
        }

        bool IsInitialized() const { return _initialized; }

        /// Encrypt outgoing SMSG header (4 bytes: Size[2 BE] + Opcode[2 LE])
        void EncryptSend(uint8* header, size_t len) {
            if (!_initialized) return;
            _encrypt.Process(header, len);
        }

        /// Decrypt incoming CMSG header (6 bytes: Size[2 BE] + Opcode[4 LE])
        void DecryptRecv(uint8* header, size_t len) {
            if (!_initialized) return;
            _decrypt.Process(header, len);
        }

    private:
        ARC4 _encrypt;
        ARC4 _decrypt;
        bool _initialized = false;
    };

} // namespace Firelands
