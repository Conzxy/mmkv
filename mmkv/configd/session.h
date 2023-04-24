// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#ifndef MMKV_CONFIG_SESSION_H__
#define MMKV_CONFIG_SESSION_H__

#include <kanon/util/noncopyable.h>
#include <kanon/net/user_server.h>

#include "mmkv/protocol/mmbp_codec.h"

namespace mmkv {

namespace server {
class Tracker;
}

namespace configd {

using protocol::MmbpCodec;


class Session : kanon::noncopyable {
 public:
  Session(server::Tracker *tracker, TcpConnectionPtr const &conn); 

 private:
  TcpConnection *conn_; 
  MmbpCodec codec_;
};

} // configd
} // mmkv

#endif // MMKV_CONFIG_SESSION_H__
