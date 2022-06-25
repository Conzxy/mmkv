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
  MmkvSession(TcpConnectionPtr const& conn, MmkvServer* server);
  ~MmkvSession() noexcept;
   
 private:
  void OnMmbpRequest(TcpConnectionPtr const& conn, std::unique_ptr<protocol::MmbpMessage> message, TimeStamp recv_time);

  TcpConnection* conn_;
  protocol::MmbpCodec codec_;
  MmkvServer* server_;

};

} // server
} // mmkv

#endif // _MMKV_SERVER_SESSION_H_
