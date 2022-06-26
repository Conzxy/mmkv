#ifndef _MMKV_PROTOCOL_COMMAND_H_
#define _MMKV_PROTOCOL_COMMAND_H_

#include <stdint.h>
#include <string>

namespace mmkv {
namespace protocol {

enum Command : uint16_t {
  STR_ADD = 0,
  STR_DEL,
  STR_GET,
  STR_SET,
  MEM_STAT,
  EXPIRE_AT,
  EXPIRE_AFTER,
  COMMAND_NUM,
};

extern std::string command_strings[];

} // protocol
} // mmkv

#endif // _MMKV_PROTOCOL_COMMAND_H_
