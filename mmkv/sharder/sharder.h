#ifndef MMKV_SERVER_SHARD_SERVER_H_
#define MMKV_SERVER_SHARD_SERVER_H_

#include <kanon/net/user_server.h>

namespace mmkv {
namespace server {

class Sharder : kanon::noncopyable {
 public:
  explicit Sharder(EventLoop *loop);
  Sharder(EventLoop *loop, InetAddr const &addr);
  
  void Listen() { server_.StartRun(); }
 private:
  TcpServer server_;
};

} // server
} // mmkv

#endif // MMKV_SERVER_SHARD_SERVER_H_