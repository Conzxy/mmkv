#ifndef _MMKV_CLIENT_INFORMATION_H_
#define _MMKV_CLIENT_INFORMATION_H_

#include <kanon/net/user_common.h>
#include <string>
#include <unordered_map>

#include <kanon/string/string_view.h>
#include <xxhash.h>

#include "mmkv/protocol/command.h"

struct StringViewHash {
  uint64_t operator()(kanon::StringView const& view) const noexcept {
    return XXH64(view.data(), view.size(), 0);
  }
};

extern std::string HELP_INFORMATION;
extern std::string command_hints[];
extern std::unordered_map<kanon::StringView, mmkv::protocol::Command, StringViewHash> command_map;

#define APPLICATION_INFORMATION \
  "Mmkv is an In-Memory Key-value remote database(or cache)\n" \
  "Copyright(c) 2022.6 Conzxy\n" \
  "Supported data structure(temporary): \n" \
  "[string]\n" \
  "[hash set](later)\n" \
  "[sequential list](later)\n" \
  "[sorted set](later)"


#endif // _MMKV_CLIENT_INFORMATION_H_
