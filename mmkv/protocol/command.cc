#include "command.h"

using namespace mmkv::protocol;

std::string mmkv::protocol::command_strings[] = {
  "stradd",
  "strdel",
  "strget",
  "strset",
  "memory_stat",
  "expire_at",
  "expire_after",
};
