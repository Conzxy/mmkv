#ifndef MMKV_SERVER_SHARD_SESSION_H_
#define MMKV_SERVER_SHARD_SESSION_H_

#include <kanon/net/user_server.h>

#include "mmkv/protocol/mmbp_codec.h"

namespace mmkv {
namespace server {

class ShardSession : kanon::noncopyable {
 public:
  explicit ShardSession(TcpConnectionPtr const &conn);

 private:
  void OnMessage(TcpConnectionPtr const &conn,
    Buffer &buffer,
    uint32_t, TimeStamp recv_time);

  TcpConnection *conn_;
  protocol::MmbpCodec codec_;
};

} // server
} // mmkv

#endif 