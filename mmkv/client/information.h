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

enum CommandFormat : uint8_t {
  F_ONLY_KEY = 0, // command key
  F_VALUE,        // command key value
  F_VALUES,       // command key values...
  F_COUNT,        // command key count
  F_RANGE,        // command key range(integer)
  F_DRANGE,       // command key range(double)
  F_VSET_MEMBERS, // vadd <weight, member>...
  F_MAP_VALUES,   // madd <field, value>...
  F_FIELD_VALUE,  // mset field value
  F_SET_OP,       // sand/or/sub key1 key2
  F_SET_OP_TO,    // sand/or/subto dest key1 key2
  F_HELP,         // help
  F_EXIT,         // exit/quit
  F_NONE,         // command
};

extern std::string HELP_INFORMATION;
extern std::string command_hints[];
extern std::unordered_map<kanon::StringView, CommandFormat, StringViewHash> command_formats;

// command_strings都是在data segment上的变量
// 故key为StringView也无妨
extern std::unordered_map<kanon::StringView, mmkv::protocol::Command, StringViewHash> command_map;

#define APPLICATION_INFORMATION \
  "Apache LICENSE Copyright(c) 2022.6 Conzxy\n" \
  "Mmkv is an Memory Key-value remote database(or cache)\n" \
  "Supported data structure: \n" \
  "Github homepage: https://github.com/Conzxy/mmkv\n" \
  "[string]\n" \
  "[list]\n" \
  "[sorted set]\n" \
  "[map]\n" \
  "[hash set]\n"


#endif // _MMKV_CLIENT_INFORMATION_H_
