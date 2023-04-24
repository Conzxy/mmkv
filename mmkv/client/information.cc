// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#include <string>
#include <unordered_map>

#include "information.h"

#include "kanon/string/string_util.h"
#include "mmkv/protocol/command.h"
#include "mmkv/util/tcolor_macro.h"

#include <kanon/log/logger.h>

using namespace mmkv::protocol;
using namespace kanon;
using namespace ::detail;
using namespace mmkv;

namespace detail {

std::string cli_command_strings[] = {"HELP",    "QUIT",  "EXIT",
                                     "HISTORY", "CLEAR", "CLEARHISTORY"};

std::string help;

std::string command_hints[COMMAND_NUM];
std::unordered_map<StringView, Command, StringViewHash> command_map;
std::unordered_map<Command, CommandFormat> command_formats;

std::string cli_command_hints[CLI_COMMAND_NUM];
std::unordered_map<kanon::StringView, CliCommand, StringViewHash>
    cli_command_map;

} // namespace detail

static KANON_INLINE int GenCommandMetadata() KANON_NOEXCEPT
{
  for (size_t i = 0; i < COMMAND_NUM; ++i) {
    command_hints[i].clear();

    switch (i) {
      case MEM_STAT:
      case KEYALL:
      case DELALL:
        command_formats[(Command)i] = F_NONE;
        command_hints[i] += "";
        break;
      case STR_ADD:
      case STR_SET:
      case STRAPPEND:
        command_formats[(Command)i] = F_VALUE;
        command_hints[i] += " key value";
        break;
      case MEXISTS:
      case MDEL:
      case MGET:
        command_formats[(Command)i] = F_VALUE;
        command_hints[i] += " key field";
        break;
      case MGETS:
        command_formats[(Command)i] = F_VALUES;
        command_hints[i] += " key fields...";
        break;
      case MSET:
        command_formats[(Command)i] = F_FIELD_VALUE;
        command_hints[i] += " key field value";
        break;
      case SAND:
      case SOR:
      case SSUB:
      case SANDSIZE:
      case SORSIZE:
      case SSUBSIZE:
        command_formats[(Command)i] = F_SET_OP;
        command_hints[i] += " key1 key2";
        break;
      case SANDTO:
      case SORTO:
      case SSUBTO:
        command_formats[(Command)i] = F_SET_OP_TO;
        command_hints[i] += " destination key1 key2";
        break;
      case SADD:
        command_formats[(Command)i] = F_VALUES;
        command_hints[i] += " key members...";
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
      case SRANDDELM:
      case PERSIST:
      case EXPIRATION:
      case TTL:
        command_formats[(Command)i] = F_ONLY_KEY;
        command_hints[i] += " key";
        break;
      case LADD:
      case LAPPEND:
      case LPREPEND:
        command_formats[(Command)i] = F_VALUES;
        command_hints[i] += " key values...";
        break;
      case STRPOPBACK:
      case LPOPBACK:
      case LPOPFRONT:
        command_formats[(Command)i] = F_COUNT;
        command_hints[i] += " key count";
        break;
      case LGETRANGE:
        command_formats[(Command)i] = F_RANGE;
        command_hints[i] += " key index_range(integer)";
        break;
      case VADD:
        command_formats[(Command)i] = F_VSET_MEMBERS;
        command_hints[i] += " key <weight, member>...";
        break;
      case MADD:
        command_formats[(Command)i] = F_MAP_VALUES;
        command_hints[i] += " key <field, value>...";
        break;
      case VDELM:
      case VWEIGHT:
      case VORDER:
      case VRORDER:
      case SDELM:
      case SEXISTS:
        command_formats[(Command)i] = F_VALUE;
        command_hints[i] += " key member";
        break;
      case VDELMRANGE:
      case VRANGE:
      case VRRANGE:
        command_formats[(Command)i] = F_RANGE;
        command_hints[i] += " key order_range(integer)";
        break;
      case VDELMRANGEBYWEIGHT:
      case VRANGEBYWEIGHT:
      case VRRANGEBYWEIGHT:
      case VSIZEBYWEIGHT:
        command_formats[(Command)i] = F_DRANGE;
        command_hints[i] += " key weight_range(double)";
        break;
      case RENAME:
        command_formats[(Command)i] = F_VALUE;
        command_hints[i] += " key new_name";
        break;
      case EXPIRE_AT:
      case EXPIREM_AT:
        command_formats[(Command)i] = F_EXPIRE;
        command_hints[i] += " key expiration_time";
        break;
      case EXPIRE_AFTER:
      case EXPIREM_AFTER:
        command_formats[(Command)i] = F_EXPIRE;
        command_hints[i] += " key time_interval";
        break;
      case DELS:
        command_formats[(Command)i] = F_MUL_KEYS;
        command_hints[i] += " keys...";
        break;
      default:
        break;
    }
  }

  for (int i = 0; i < CLI_COMMAND_NUM; ++i) {
    cli_command_hints[i].clear();
    switch (i) {
      case CLI_HELP:
      case CLI_QUIT:
      case CLI_EXIT:
      case CLI_HISTORY:
      case CLI_CLEAR:
      case CLI_CLEAR_HISTORY:
        cli_command_hints[i] += "";
        break;

      default:
        LOG_FATAL << "Unknown Cli command, unable to register its hint";
    }
  }
  return 0;
}

static int GenHelp()
{
  assert(help.empty());
  help = "Help: \n";
  for (int i = 0; i < CLI_COMMAND_NUM; ++i) {
    StrAppend(help, GREEN, GetCliCommandString((CliCommand)i), RESET,
              GetCliCommandHint((CliCommand)i), "\n");
  }

  for (int i = 0; i < COMMAND_NUM; ++i) {
    StrAppend(help, L_GREEN, GetCommandString((Command)i), RESET,
              GetCommandHint((Command)i), "\n");
  }
  return 0;
}

static KANON_INLINE int GenCommandMap() KANON_NOEXCEPT
{
  for (size_t i = 0; i < COMMAND_NUM; ++i) {
    command_map[GetCommandString((Command)i)] = (Command)i;
  }

  for (size_t i = 0; i < CLI_COMMAND_NUM; ++i) {
    cli_command_map[GetCliCommandString((CliCommand)i)] = (CliCommand)i;
  }

  return 0;
}

void InstallInformation() KANON_NOEXCEPT
{
  GenCommandMetadata();
  GenCommandMap();
  GenHelp();
}
