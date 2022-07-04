#include <string>
#include <unordered_map>

#include "information.h"
#include "mmkv/protocol/command.h"

using namespace mmkv::protocol;
using namespace kanon;

std::string command_hints[COMMAND_NUM];
std::string HELP_INFORMATION;
std::unordered_map<StringView, Command, StringViewHash> command_map;

static inline int GenCommandHint() {
  for (size_t i = 0; i < COMMAND_NUM; ++i) {
    command_hints[i].clear();
    command_hints[i] += command_strings[i];

    switch (i) {
      case STR_ADD:
      case STR_SET:
        command_hints[i] += " key value";
        break;
      case STR_GET:
      case STR_DEL:
      case LGETALL:
      case LGETSIZE:
      case LDEL:
        command_hints[i] += " key";
        break;
      case LADD:
      case LAPPEND:
      case LPREPEND:
        command_hints[i] += " key values...";
        break;
      case LPOPBACK:
      case LPOPFRONT:
        command_hints[i] += " key count";
        break;
      case LGETRANGE:
        command_hints[i] += " key left right";
        break;
      default:
        break;
    }
  }

  return 0;
}

static int dummy = GenCommandHint();

static int GenHelp() {
  HELP_INFORMATION += "Help: \n";
  
  HELP_INFORMATION += "help\n";
  HELP_INFORMATION += "exit/quit\n";

  for (auto const& hint : command_hints) {
    HELP_INFORMATION += hint;
    HELP_INFORMATION += "\n";
  }
  
  return 0;
}

static int dummy2 = GenHelp();

static int GenCommandMap() {
  for (size_t i = 0; i < COMMAND_NUM; ++i) {
    command_map[command_strings[i]] = (Command)i;
  }

  return 0;
}

static int dummy3 = GenCommandMap();
