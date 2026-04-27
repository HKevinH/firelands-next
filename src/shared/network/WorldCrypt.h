#pragma once

#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <shared/Common.h>
#include <vector>
#include <algorithm>
#include <cstring>

namespace Firelands {

/**
 * @brief ARC4 stream cipher implementation.
 */
class ARC4 {
public:
  void Init(const uint8 *key, size_t keyLen) {
    for (int i = 0; i < 256; ++i)
      _s[i] = static_cast<uint8>(i);
    uint8 j = 0;
    for (int i = 0; i < 256; ++i) {
      j = static_cast<uint8>(j + _s[i] + key[i % keyLen]);
      std::swap(_s[i], _s[j]);
    }
    _i = _j = 0;
  }

  void Process(uint8 *data, size_t len) {
    for (size_t k = 0; k < len; ++k) {
      _i = static_cast<uint8>(_i + 1);
      _j = static_cast<uint8>(_j + _s[_i]);
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
 */
class WorldCrypt {
public:
  void Init(const std::vector<uint8_t> &sessionKey) {
    static const uint8_t ServerEncryptionKeySeed[] = { 0xCC, 0x98, 0xAE, 0x04, 0xE8, 0x97, 0xEA, 0xCA, 0x12, 0xDD, 0xC0, 0x93, 0x42, 0x91, 0x53, 0x57 };
    static const uint8_t ServerDecryptionKeySeed[] = { 0xC2, 0xB3, 0x72, 0x3C, 0xC6, 0xAE, 0xD9, 0xB5, 0x34, 0x3C, 0x53, 0xEE, 0x2F, 0x43, 0x67, 0xCE };

    uint8_t encryptHash[20], decryptHash[20];
    unsigned int hashLen = 20;

    // HMAC(evp, key, key_len, data, data_len, digest, digest_len)
    // REFERENCIA: Key = Seed, Data = SessionKey (K)
    HMAC(EVP_sha1(), ServerEncryptionKeySeed, sizeof(ServerEncryptionKeySeed),
         sessionKey.data(), static_cast<int>(sessionKey.size()), encryptHash, &hashLen);
    HMAC(EVP_sha1(), ServerDecryptionKeySeed, sizeof(ServerDecryptionKeySeed),
         sessionKey.data(), static_cast<int>(sessionKey.size()), decryptHash, &hashLen);

    _encrypt.Init(encryptHash, 20);
    _decrypt.Init(decryptHash, 20);

    // Drop 1024 bytes (ARC4-drop1024)
    uint8 drop[1024];
    std::memset(drop, 0, 1024);
    _encrypt.Process(drop, 1024);
    std::memset(drop, 0, 1024);
    _decrypt.Process(drop, 1024);

    _initialized = true;
  }

  bool IsInitialized() const { return _initialized; }

  void EncryptSend(uint8 *header, size_t len) {
    if (_initialized) _encrypt.Process(header, len);
  }

  void DecryptRecv(uint8 *header, size_t len) {
    if (_initialized) _decrypt.Process(header, len);
  }

private:
  ARC4 _encrypt;
  ARC4 _decrypt;
  bool _initialized = false;
};

} // namespace Firelands
