// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#ifndef _MMKV_CLIENT_TRASLATOR_H_
#define _MMKV_CLIENT_TRASLATOR_H_

#include "mmkv/protocol/mmbp.h"
#include "mmkv/protocol/mmbp_request.h"

#include <kanon/string/string_view.h>

namespace mmkv {
namespace protocol {

class Translator {
 public:
  enum ErrorCode : uint8_t {
    E_OK = 0,
    E_SYNTAX_ERROR,
    E_NO_COMMAND,
  };

  Translator() = default;
  ~Translator() = default;

  ErrorCode Parse(MmbpRequest *message, Command cmd,
                  kanon::StringView statement);

 private:
};

} // namespace protocol
} // namespace mmkv

#endif // _MMKV_CLIENT_TRASLATOR_H_
