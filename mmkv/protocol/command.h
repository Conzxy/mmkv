#ifndef _MMKV_PROTOCOL_COMMAND_H_
#define _MMKV_PROTOCOL_COMMAND_H_

#include <stdint.h>

namespace mmkv {
namespace protocol {

enum Command : uint16_t {
  STR_ADD = 0,
  STR_DEL,
  STR_GET,
  STR_SET,
  MEM_STAT,
  SET_EXPIRE,
};

} // protocol
} // mmkv

#endif // _MMKV_PROTOCOL_COMMAND_H_
