#ifndef MMKV_SERVER_ROUTER_SESSINO_H__
#define MMKV_SERVER_ROUTER_SESSINO_H__

#include <kanon/util/noncopyable.h>
#include <kanon/net/user_server.h>

#include "mmkv/protocol/mmbp_codec.h"

namespace mmkv {
namespace server {

using protocol::MmbpCodec;

class Tracker;

class RouterSession : kanon::noncopyable {
 public:
  RouterSession(Tracker *tracker, TcpConnectionPtr const &conn); 

 private:
  TcpConnection *conn_; 
  MmbpCodec codec_;
};

} // server
} // mmkv

#endif // MMKV_SERVER_ROUTER_SESSINO_H__
