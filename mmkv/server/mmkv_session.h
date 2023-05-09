// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#ifndef _MMKV_SERVER_MMKV_SESSION_H_
#define _MMKV_SERVER_MMKV_SESSION_H_

#include "kanon/util/noncopyable.h"
#include "kanon/net/user_common.h"

#include "mmkv/protocol/mmbp.h"
#include "mmkv/protocol/mmbp_codec.h"
#include <kanon/net/callback.h>

namespace mmkv {
namespace server {

class MmkvServer;

class MmkvSession {
  DISABLE_EVIL_COPYABLE(MmkvSession)

 public:
  MmkvSession(TcpConnectionPtr const &conn, MmkvServer *server);
  ~MmkvSession() noexcept;

 private:
  TcpConnection *conn_;
  MmkvServer    *server_;
};

} // namespace server
} // namespace mmkv

#endif // _MMKV_SERVER_SESSION_H_
