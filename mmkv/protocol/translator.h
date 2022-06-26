#ifndef _MMKV_PROTOCOL_TRASLATOR_H_
#define _MMKV_PROTOCOL_TRASLATOR_H_

#include "mmbp_request.h"
#include "mmkv/protocol/mmbp.h"

#include <stdint.h>
#include <vector>
#include <kanon/string/string_view.h>

namespace mmkv {
namespace protocol {

class Translator {
  enum ErrorCode : uint8_t {
    E_OK = 0,
    E_INVALID_COMMAND,
  };

 public:
  Translator() = default;
  ~Translator() = default;
  
  ErrorCode Parse(MmbpRequest* message, kanon::StringView statement);
 private:

};

} // protocol
} // mmkv

#endif // _MMKV_PROTOCOL_TRASLATOR_H_
