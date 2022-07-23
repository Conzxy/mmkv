#ifndef _CHISATO_CHISATO_H_
#define _CHISATO_CHISATO_H_

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <string>
#include <unordered_map>

namespace chisato {

template<typename K, typename V>
using Map = std::unordered_map<K, V>;

static Map<std::string, std::string> field_value_map;

#define MAX_LINE_LEN 4096

inline bool ReadLine(FILE* file, std::string& line, const bool need_newline=true) {
  char buf[MAX_LINE_LEN];
  char* ret = NULL;
  size_t n = 0;

  line.clear();

  do {
    ret = ::fgets(buf, sizeof buf, file);

    if (ret == NULL) {
      if (::ferror(file)) {
        if (errno == EINTR) {
          continue;
        }
      }

      return false;
    }

    assert(ret == buf);
    n = strlen(buf);

    if (n >= 1 && buf[n-1] == '\n') {
      if (!need_newline) {
        if (n >= 2 && buf[n-2] == '\r') {
          line.append(buf, n - 2);
        }
        else {
          line.append(buf, n - 1);
        }
      } else {
        line.append(buf, n);
      }
      break;
    }

    line.append(buf, n);

  } while (n == (sizeof(buf) - 1));

  return true;
}

inline bool Parse(char const* path) {
  FILE* file = ::fopen(path, "r");

  if (!file) {
    ::fprintf(stderr, "Failed to open the config file: %s\n", path);
    ::fprintf(stderr, "Error message: %s\n", strerror(errno));
    return false;
  }

  std::string line;

  while (ReadLine(file, line, false)) {;
    auto colon_pos = line.find(':');
    auto comment_pos = line.find('#');

    if (comment_pos != std::string::npos) {
      continue;
    }

    if (colon_pos != std::string::npos) {
      // Read config field
      auto space_pos = line.find(' ', colon_pos+2);

      if (space_pos != std::string::npos) {
        field_value_map.emplace(line.substr(0, colon_pos), line.substr(colon_pos+2, space_pos));
      }
      else {
        field_value_map.emplace(line.substr(0, colon_pos), line.substr(colon_pos+2));
      }
    }
  }

  if (::feof(file) == 0) {
    ::fprintf(stderr, "Failed to read one line from %s\n", path);
    return false;
  }

  return true;
}

inline bool Parse(std::string const& path) {
  return Parse(path.c_str());
}

inline std::string GetField(std::string const& field) {
  auto iter = field_value_map.find(field);

  if (iter == field_value_map.end()) {
    return "";
  }

  return std::move(iter->second);
}

} // namespace chisato

#endif
