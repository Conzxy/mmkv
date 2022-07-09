#include <string>
#include <unordered_map>

#include "information.h"
#include "mmkv/protocol/command.h"

using namespace mmkv::protocol;
using namespace kanon;

std::string command_hints[COMMAND_NUM];
std::string HELP_INFORMATION;
std::unordered_map<StringView, Command, StringViewHash> command_map;
std::unordered_map<StringView, CommandFormat, StringViewHash> command_formats;

static inline int GenCommandMetadata() {
  for (size_t i = 0; i < COMMAND_NUM; ++i) {
    command_hints[i].clear();
    command_hints[i] += command_strings[i];

    command_formats["exit"] = F_EXIT;
    command_formats["quit"] = F_EXIT;
    command_formats["help"] = F_HELP;

    switch (i) {
      case MEM_STAT:
        command_formats[command_strings[i]] = F_NONE;
        break;
      case STR_ADD:
      case STR_SET:
        command_formats[command_strings[i]] = F_VALUE;
        command_hints[i] += " key value";
        break;
      case MEXISTS:
      case MDEL:
      case MGET:
        command_formats[command_strings[i]] = F_VALUE;
        command_hints[i] += " key field";
        break;
      case MGETS:
        command_formats[command_strings[i]] = F_VALUES;
        command_hints[i] += " key fields...";
        break;
      case MSET:
        command_formats[command_strings[i]] = F_FIELD_VALUE;
        command_hints[i] += " key field value";
        break;
      case STR_GET:
      case STR_DEL:
      case LGETALL:
      case LGETSIZE:
      case LDEL:
      case VSIZE:
      case VALL:
      case DEL:
      case TYPE:
      case MALL:
      case MSIZE:
      case MFIELDS:
      case MVALUES:
        command_formats[command_strings[i]] = F_ONLY_KEY;
        command_hints[i] += " key";
        break;
      case LADD:
      case LAPPEND:
      case LPREPEND:
        command_formats[command_strings[i]] = F_VALUES;
        command_hints[i] += " key values...";
        break;
      case LPOPBACK:
      case LPOPFRONT:
        command_formats[command_strings[i]] = F_COUNT;
        command_hints[i] += " key count";
        break;
      case LGETRANGE:
        command_formats[command_strings[i]] = F_RANGE;
        command_hints[i] += " key index_range(integer)";
        break;
      case VADD:
        command_formats[command_strings[i]] = F_VSET_MEMBERS;
        command_hints[i] += " key <weight, member>...";
        break;
      case MADD:
        command_formats[command_strings[i]] = F_MAP_VALUES;
        command_hints[i] += " key <field, value>...";
        break;
      case VDELM:
      case VWEIGHT:
      case VORDER:
      case VRORDER:
        command_formats[command_strings[i]] = F_VALUE;
        command_hints[i] += " key member";
        break;
      case VDELMRANGE:
      case VRANGE:
      case VRRANGE:
        command_formats[command_strings[i]] = F_RANGE;
        command_hints[i] += " key order_range(integer)";
        break;
      case VDELMRANGEBYWEIGHT:
      case VRANGEBYWEIGHT:
      case VRRANGEBYWEIGHT:
      case VSIZEBYWEIGHT:
        command_formats[command_strings[i]] = F_DRANGE;
        command_hints[i] += " key weight_range(double)";
        break;
      case RENAME:
        command_formats[command_strings[i]] = F_VALUE;
        command_hints[i] += " key new_name";
        break;

      default:
        break;
    }
  }

  return 0;
}

static int dummy = GenCommandMetadata();

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
