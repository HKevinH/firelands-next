#ifndef FIRELANDS_SHARED_CRYPTO_H
#define FIRELANDS_SHARED_CRYPTO_H

#include <algorithm>
#include <iomanip>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <sstream>
#include <string>
#include <vector>

namespace Firelands {
namespace Crypto {

using SHA1Hash = std::vector<uint8_t>;

inline std::string ToUpper(std::string str) {
  std::transform(str.begin(), str.end(), str.begin(), ::toupper);
  return str;
}

inline SHA1Hash CalculateSHA1(const std::string &input) {
  SHA1Hash hash(SHA_DIGEST_LENGTH);
  EVP_MD_CTX *ctx = EVP_MD_CTX_new();
  EVP_DigestInit_ex(ctx, EVP_sha1(), nullptr);
  EVP_DigestUpdate(ctx, input.c_str(), input.length());
  unsigned int len = 0;
  EVP_DigestFinal_ex(ctx, hash.data(), &len);
  EVP_MD_CTX_free(ctx);
  return hash;
}

inline SHA1Hash CalculateSHA1(const std::vector<uint8_t> &input) {
  SHA1Hash hash(SHA_DIGEST_LENGTH);
  EVP_MD_CTX *ctx = EVP_MD_CTX_new();
  EVP_DigestInit_ex(ctx, EVP_sha1(), nullptr);
  EVP_DigestUpdate(ctx, input.data(), input.size());
  unsigned int len = 0;
  EVP_DigestFinal_ex(ctx, hash.data(), &len);
  EVP_MD_CTX_free(ctx);
  return hash;
}

inline std::string ToHexString(const uint8_t *data, size_t len) {
  std::stringstream ss;
  ss << std::hex << std::setfill('0') << std::uppercase;
  for (size_t i = 0; i < len; ++i) {
    ss << std::setw(2) << static_cast<int>(data[i]);
  }
  return ss.str();
}

inline std::string ToHexString(const SHA1Hash &hash) {
  return ToHexString(hash.data(), hash.size());
}

inline std::vector<uint8_t> FromHexString(const std::string &hex) {
  std::vector<uint8_t> bytes;
  for (unsigned int i = 0; i < hex.length(); i += 2) {
    std::string byteString = hex.substr(i, 2);
    uint8_t byte = (uint8_t)strtoul(byteString.c_str(), nullptr, 16);
    bytes.push_back(byte);
  }
  return bytes;
}

class SHA1 {
public:
  SHA1() {
    _ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(_ctx, EVP_sha1(), nullptr);
  }
  ~SHA1() { EVP_MD_CTX_free(_ctx); }
  void Update(const uint8_t *data, size_t len) {
    EVP_DigestUpdate(_ctx, data, len);
  }
  template <typename T> void Update(T val) {
    Update((const uint8_t *)&val, sizeof(T));
  }
  void Update(const std::string &str) {
    Update((const uint8_t *)str.c_str(), str.length());
  }
  void Update(const std::vector<uint8_t> &vec) {
    Update(vec.data(), vec.size());
  }
  SHA1Hash Finalize() {
    SHA1Hash hash(SHA_DIGEST_LENGTH);
    unsigned int len = 0;
    EVP_DigestFinal_ex(_ctx, hash.data(), &len);
    // Re-initialize for reuse if needed, or just let users create new ones
    return hash;
  }

private:
  EVP_MD_CTX *_ctx;
};

} // namespace Crypto
} // namespace Firelands

#endif // FIRELANDS_SHARED_CRYPTO_H
