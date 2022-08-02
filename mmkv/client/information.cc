#include <string>
#include <unordered_map>

#include "information.h"

#include "mmkv/protocol/command.h"
#include "mmkv/util/tcolor_macro.h"

#include <kanon/log/logger.h>

using namespace mmkv::protocol;
using namespace kanon;
using namespace ::detail;

namespace detail {

std::string command_hints[COMMAND_NUM];
std::string help;
std::unordered_map<StringView, Command, StringViewHash> command_map;
std::unordered_map<StringView, CommandFormat, StringViewHash> command_formats;

} // detail


static inline int GenCommandMetadata() {
  command_formats["EXIT"] = F_EXIT;
  command_formats["QUIT"] = F_EXIT;
  command_formats["HELP"] = F_HELP;

  for (size_t i = 0; i < COMMAND_NUM; ++i) {
    command_hints[i].clear();
    command_hints[i] += GetCommandString((Command)i);

    switch (i) {
      case MEM_STAT:
      case KEYALL:
        command_formats[GetCommandString((Command)i)] = F_NONE;
        command_hints[i] = "";
        break;
      case STR_ADD:
      case STR_SET:
      case STRAPPEND:
        command_formats[GetCommandString((Command)i)] = F_VALUE;
        command_hints[i] = " key value";
        break;
      case MEXISTS:
      case MDEL:
      case MGET:
        command_formats[GetCommandString((Command)i)] = F_VALUE;
        command_hints[i] = " key field";
        break;
      case MGETS:
        command_formats[GetCommandString((Command)i)] = F_VALUES;
        command_hints[i] = " key fields...";
        break;
      case MSET:
        command_formats[GetCommandString((Command)i)] = F_FIELD_VALUE;
        command_hints[i] = " key field value";
        break;
      case SAND:
      case SOR:
      case SSUB:
      case SANDSIZE:
      case SORSIZE:
      case SSUBSIZE:
        command_formats[GetCommandString((Command)i)] = F_SET_OP;
        command_hints[i] = " key1 key2";
        break;
      case SANDTO:
      case SORTO:
      case SSUBTO:
        command_formats[GetCommandString((Command)i)] = F_SET_OP_TO;
        command_hints[i] = " destination key1 key2";
        break;
      case SADD:
        command_formats[GetCommandString((Command)i)] = F_VALUES;
        command_hints[i] = " key members...";
        break;
      case STR_GET:
      case STR_DEL:
      case STRLEN:
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
      case SALL:
      case SSIZE:
        command_formats[GetCommandString((Command)i)] = F_ONLY_KEY;
        command_hints[i] = " key";
        break;
      case LADD:
      case LAPPEND:
      case LPREPEND:
        command_formats[GetCommandString((Command)i)] = F_VALUES;
        command_hints[i] = " key values...";
        break;
      case STRPOPBACK:
      case LPOPBACK:
      case LPOPFRONT:
        command_formats[GetCommandString((Command)i)] = F_COUNT;
        command_hints[i] = " key count";
        break;
      case LGETRANGE:
        command_formats[GetCommandString((Command)i)] = F_RANGE;
        command_hints[i] = " key index_range(integer)";
        break;
      case VADD:
        command_formats[GetCommandString((Command)i)] = F_VSET_MEMBERS;
        command_hints[i] = " key <weight, member>...";
        break;
      case MADD:
        command_formats[GetCommandString((Command)i)] = F_MAP_VALUES;
        command_hints[i] = " key <field, value>...";
        break;
      case VDELM:
      case VWEIGHT:
      case VORDER:
      case VRORDER:
      case SDELM:
      case SEXISTS:
        command_formats[GetCommandString((Command)i)] = F_VALUE;
        command_hints[i] = " key member";
        break;
      case VDELMRANGE:
      case VRANGE:
      case VRRANGE:
        command_formats[GetCommandString((Command)i)] = F_RANGE;
        command_hints[i] = " key order_range(integer)";
        break;
      case VDELMRANGEBYWEIGHT:
      case VRANGEBYWEIGHT:
      case VRRANGEBYWEIGHT:
      case VSIZEBYWEIGHT:
        command_formats[GetCommandString((Command)i)] = F_DRANGE;
        command_hints[i] = " key weight_range(double)";
        break;
      case RENAME:
        command_formats[GetCommandString((Command)i)] = F_VALUE;
        command_hints[i] = " key new_name";
        break;
      case EXPIRE_AT:
      case EXPIREM_AT:
        command_formats[GetCommandString(Command(i))] = F_EXPIRE;
        command_hints[i] = " key expiration_time";
        break;
      case EXPIRE_AFTER:
      case EXPIREM_AFTER:
        command_formats[GetCommandString((Command)i)] = F_EXPIRE;
        command_hints[i] = " key time_interval";
        break;
      default:
        break;
    }
  }

  return 0;
}

// static int dummy = GenCommandMetadata();

static int GenHelp() {
  help += "Help: \n";

  help += GREEN "help\n" RESET;
  help += GREEN "exit/quit\n" RESET;

  for (int i = 0; i < COMMAND_NUM; ++i) {
    help += L_GREEN;
    help += GetCommandString((Command)i);
    help += RESET;
    help += GetCommandHint((Command)i);
    help += "\n";
  }
  
  return 0;
}

// static int dummy2 = GenHelp();

static int GenCommandMap() {
  for (size_t i = 0; i < COMMAND_NUM; ++i) {
    command_map[GetCommandString((Command)i)] = (Command)i;
  }

  // COMMAND_NUM as the invalid indicator
  command_map["QUIT"] = COMMAND_NUM;
  command_map["EXIT"] = COMMAND_NUM;
  command_map["HELP"] = COMMAND_NUM;

  return 0;
}

// static int dummy3 = GenCommandMap();

void InstallInformation() noexcept {
  GenCommandMetadata();
  GenHelp();
  GenCommandMap();
}

