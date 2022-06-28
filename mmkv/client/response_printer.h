#ifndef _MMKV_CLIENT_RESPONSE_PRINTER_H_
#define _MMKV_CLIENT_RESPONSE_PRINTER_H_

#include "mmkv/protocol/mmbp_response.h"

#include <kanon/util/noncopyable.h>

namespace mmkv {
namespace client {

class ResponsePrinter {
  DISABLE_EVIL_COPYABLE(ResponsePrinter)

 public:
  ResponsePrinter() = default;

  void Printf(protocol::MmbpResponse* response);
 private:

};

} // client
} // mmkv

#endif // _MMKV_CLIENT_RESPONSE_PRINTER_H_
