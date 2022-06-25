#ifndef _MMKV_PROTOCOL_ERROR_CODE_H_
#define _MMKV_PROTOCOL_ERROR_CODE_H_

#include <stdint.h>

namespace mmkv {
namespace protocol {

enum StatusCode : uint8_t {
  S_OK = 0,
  S_EXISTS,
  S_NONEXISTS,
  S_INVALID_MESSAGE,
};

char const* GetStatusMessage(StatusCode code) noexcept;

} // protocol
} // mmkv

#endif // _MMKV_PROTOCOL_ERROR_CODE_H_
