#ifndef FIRELANDS_APPLICATION_SERVICES_COMMAND_TEXT_UTILS_H
#define FIRELANDS_APPLICATION_SERVICES_COMMAND_TEXT_UTILS_H

#include <cctype>
#include <cstring>
#include <string>
#include <vector>

namespace Firelands {

/// Join argument tokens with single spaces. Shared by several command handlers.
inline std::string JoinArgs(std::vector<std::string>::const_iterator begin,
                            std::vector<std::string>::const_iterator end) {
  std::string out;
  for (auto it = begin; it != end; ++it) {
    if (!out.empty())
      out += ' ';
    out += *it;
  }
  return out;
}

/// Case-insensitive ASCII compare of `a` against the null-terminated `b`.
inline bool AsciiEqualsLower(std::string const &a, char const *b) {
  size_t const n = std::strlen(b);
  if (a.size() != n)
    return false;
  for (size_t i = 0; i < n; ++i) {
    if (std::tolower(static_cast<unsigned char>(a[i])) !=
        static_cast<unsigned char>(b[i])) {
      return false;
    }
  }
  return true;
}

/// True when `s` is non-empty and every character is an ASCII digit.
inline bool IsAllDigitAscii(std::string const &s) {
  if (s.empty())
    return false;
  for (unsigned char c : s) {
    if (!std::isdigit(c))
      return false;
  }
  return true;
}

} // namespace Firelands

#endif // FIRELANDS_APPLICATION_SERVICES_COMMAND_TEXT_UTILS_H
