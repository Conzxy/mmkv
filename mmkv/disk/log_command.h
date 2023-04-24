// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#ifndef _MMKV_DISK_LOG_COMMAND_H_
#define _MMKV_DISK_LOG_COMMAND_H_

#include "mmkv/protocol/command.h"

namespace mmkv {
namespace disk {

enum CommandType : uint8_t {
  CT_READ = 0,
  CT_WRITE,
  CT_INVALID,
};

namespace detail {

extern CommandType g_command_type[protocol::COMMAND_NUM];

} // detail

inline CommandType GetCommandType(protocol::Command cmd) noexcept {
  if (cmd < protocol::Command::COMMAND_NUM)
    return detail::g_command_type[cmd];
  return CT_INVALID;
}

} // disk
} // mmkv

#endif