// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#ifndef _MMKV_CLIENT_INFORMATION_H_
#define _MMKV_CLIENT_INFORMATION_H_

#include <functional>
#include <kanon/string/string_view.h>
#include <string>
#include <unordered_map>
#include <xxhash.h>

#include "kanon/util/macro.h"
#include "mmkv/protocol/command.h"
#include "mmkv/version.h"

using mmkv::protocol::Command;

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
  F_EXPIRE,       // expirexxx expiration
  F_NONE,         // command
  F_MUL_KEYS,     // command keys...
  F_INVALID,      // Invalid command
};

#define APPLICATION_INFORMATION                                                \
  "Apache LICENSE Copyright(c) 2022.6 Conzxy\n"                                \
  "Mmkv is an Memory Key-value remote database(or cache)\n"                    \
  "Github page: https://github.com/Conzxy/mmkv\n"                              \
  "Version: " MMKV_VERSION_STR "\n"                                            \
  "Supported data structure: \n"                                               \
  "[string]\n"                                                                 \
  "[list]\n"                                                                   \
  "[sorted set]\n"                                                             \
  "[map]\n"                                                                    \
  "[hash set]\n\n"                                                             \
  "Type help to check all the supported commands\n"

/**
 * Represents CLI specfic command.
 * To distinguish Mmkv command and CLI specific command.
 */
enum CliCommand {
  CLI_HELP = 0,
  CLI_QUIT,
  CLI_EXIT,
  CLI_HISTORY,
  CLI_CLEAR,
  CLI_CLEAR_HISTORY,
  CLI_COMMAND_NUM,
};

namespace detail {

struct StringViewHash {
  uint64_t operator()(kanon::StringView const &view) const noexcept
  {
    return XXH64(view.data(), view.size(), 0);
  }
};

extern std::string help;

extern std::string command_hints[];

// command_strings都是在data segment上的变量
// 故key为StringView也无妨
extern std::unordered_map<kanon::StringView, Command, StringViewHash>
    command_map;
// extern std::unordered_map<kanon::StringView, CommandFormat, StringViewHash>
// command_formats;
extern std::unordered_map<Command, CommandFormat> command_formats;

// CLI command
extern std::string cli_command_strings[];
extern std::string cli_command_hints[];
extern std::unordered_map<kanon::StringView, CliCommand, StringViewHash>
    cli_command_map;

} // namespace detail

/*--------------------------------------------------*/
/* Mmkv Command API                                 */
/*--------------------------------------------------*/

KANON_INLINE CommandFormat GetCommandFormat(Command command)
{
  auto iter = ::detail::command_formats.find(command);
  if (iter == ::detail::command_formats.end()) return F_INVALID;
  return iter->second;
}

/**
 * (deprecated) Only used for set the command of mmbp request
 * \brief Set the \p cmd from \p command
 * \return
 *  true -- command is a valid command
 */
KANON_INLINE KANON_DEPRECATED_ATTR bool GetCommand(kanon::StringView command,
                                                   uint16_t &cmd)
{
  auto iter = ::detail::command_map.find(command);
  if (iter == ::detail::command_map.end()) return false;
  cmd = iter->second;
  return true;
}

/**
 * \brief Convert the command string to command enumeration
 */
KANON_INLINE Command GetCommand(kanon::StringView command)
{
  auto iter = ::detail::command_map.find(command);
  if (iter == ::detail::command_map.end()) return Command::COMMAND_NUM;
  return iter->second;
}

KANON_INLINE std::string const *GetCommandHints() KANON_NOEXCEPT
{
  return ::detail::command_hints;
}

KANON_INLINE std::string const &GetCommandHint(Command cmd) KANON_NOEXCEPT
{
  assert(cmd >= 0 && cmd < Command::COMMAND_NUM);
  return ::detail::command_hints[cmd];
}

/*--------------------------------------------------*/
/* CLI command API                                  */
/*--------------------------------------------------*/

KANON_INLINE std::string const &
GetCliCommandString(CliCommand cmd) KANON_NOEXCEPT
{
  assert(cmd >= 0 && cmd < CliCommand::CLI_COMMAND_NUM);
  return ::detail::cli_command_strings[cmd];
}

KANON_INLINE std::string const *GetCliCommandStrings() KANON_NOEXCEPT
{
  return ::detail::cli_command_strings;
}

KANON_INLINE std::string const &GetCliCommandHint(CliCommand cmd)
{
  assert(cmd >= 0 && cmd < CliCommand::CLI_COMMAND_NUM);
  return ::detail::cli_command_hints[cmd];
}

KANON_INLINE std::string const *GetCliCommandHints() KANON_NOEXCEPT
{
  return ::detail::cli_command_hints;
}

KANON_INLINE CliCommand GetCliCommand(kanon::StringView cmd)
{
  auto iter = ::detail::cli_command_map.find(cmd);
  if (iter == ::detail::cli_command_map.end()) {
    return CliCommand::CLI_COMMAND_NUM;
  }
  return iter->second;
}

KANON_INLINE std::string const &GetHelp() { return ::detail::help; }

void InstallInformation() noexcept;

#endif // _MMKV_CLIENT_INFORMATION_H_
