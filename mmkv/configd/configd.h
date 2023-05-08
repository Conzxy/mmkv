// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#ifndef MMKV_CONFIGD_SERVER_H_
#define MMKV_CONFIGD_SERVER_H_

#include <kanon/util/noncopyable.h>
#include <kanon/net/user_server.h>

#include "mmkv/tracker/shard_controller_server.h"

namespace mmkv {
namespace server {

/**
 *
 */
class ConfigServer : kanon::noncopyable {
 public:
  ConfigServer(EventLoop *loop, InetAddr const &addr, InetAddr const &clter_addr);
  ~ConfigServer() noexcept;

  void Listen() { server_.StartRun(); }

 private:
  TcpServer server_;

  EventLoopThread       ctler_loop_thr_;
  ShardControllerServer ctler_;
};

} // namespace server
} // namespace mmkv

#endif // MMKV_CONFIGD_SERVER_H_
