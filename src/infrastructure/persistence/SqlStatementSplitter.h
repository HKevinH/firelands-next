#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace Firelands {

/// Splits JDBC migration SQL into individual statements (semicolon delimiters).
/// Tracks single/double/backtick quoting and SQL '' / "" escapes inside literals.
inline std::vector<std::string> SplitSqlStatements(std::string const &sql) {
  std::vector<std::string> statements;
  std::string current;
  bool inString = false;
  char stringChar = 0;
  bool inLineComment = false;

  for (size_t i = 0; i < sql.length(); ++i) {
    char const c = sql[i];

    if (inLineComment) {
      current += c;
      if (c == '\n')
        inLineComment = false;
      continue;
    }

    if (!inString && c == '-' && i + 1 < sql.length() && sql[i + 1] == '-') {
      current += c;
      current += sql[++i];
      inLineComment = true;
      continue;
    }

    if ((c == '\'' || c == '"' || c == '`') &&
        (i == 0 || sql[i - 1] != '\\')) {
      if (!inString) {
        inString = true;
        stringChar = c;
      } else if (stringChar == c) {
        if (c == '\'' && i + 1 < sql.length() && sql[i + 1] == '\'') {
          current += c;
          current += sql[i + 1];
          ++i;
          continue;
        }
        if (c == '"' && i + 1 < sql.length() && sql[i + 1] == '"') {
          current += c;
          current += sql[i + 1];
          ++i;
          continue;
        }
        if (c == '`' && i + 1 < sql.length() && sql[i + 1] == '`') {
          current += c;
          current += sql[i + 1];
          ++i;
          continue;
        }
        inString = false;
      }
    }

    if (c == ';' && !inString) {
      statements.push_back(current);
      current.clear();
    } else {
      current += c;
    }
  }

  if (!current.empty())
    statements.push_back(current);
  return statements;
}

} // namespace Firelands
