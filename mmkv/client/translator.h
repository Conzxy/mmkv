#ifndef _MMKV_CLIENT_TRASLATOR_H_
#define _MMKV_CLIENT_TRASLATOR_H_

#include "mmkv/protocol/mmbp_request.h"
#include "mmkv/protocol/mmbp.h"

#include <kanon/string/string_view.h>

namespace mmkv {
namespace protocol {

class Translator {
 public:
  enum ErrorCode : uint8_t {
    E_OK = 0,
    E_INVALID_COMMAND,
    E_SYNTAX_ERROR,
    E_EXIT,
    E_NO_COMMAND,
    E_SHELL_CMD,
  };

  Translator() = default;
  ~Translator() = default;
  
  ErrorCode Parse(MmbpRequest* message, kanon::StringView statement);
 private:

};

} // protocol
} // mmkv

#endif // _MMKV_CLIENT_TRASLATOR_H_
