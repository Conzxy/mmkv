#include "command.h"

using namespace mmkv::protocol;

std::string mmkv::protocol::command_strings[] = {
  "stradd",
  "strdel",
  "strget",
  "strset",
  "memorystat",
  "expire_at",
  "expire_after",
  "ladd",
  "lappend",
  "lprepend",
  "lgetsize",
  "lgetall",
  "lgetrange",
  "lpopfront",
  "lpopback",
  "ldel",
};

