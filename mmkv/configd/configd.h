// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#ifndef MMKV_CONFIGD_SERVER_H_
#define MMKV_CONFIGD_SERVER_H_

#include <kanon/util/noncopyable.h>
#include <kanon/net/user_server.h>

#include "mmkv/tracker/shard_controller_server.h"
#include "configd_codec.h"

namespace mmkv {
namespace server {

/**
 *
 */
class Configd : kanon::noncopyable {
  friend class ShardControllerServer;

 public:
  Configd(EventLoop *loop, InetAddr const &addr, InetAddr const &clter_addr);
  ~Configd() noexcept;

  void Listen() { server_.StartRun(); }

 private:
  void SyncConfig(Configuration const &config);

  TcpServer    server_;
  ConfigdCodec codec_;

  EventLoopThread       ctler_loop_thr_;
  ShardControllerServer ctler_;
};

} // namespace server
} // namespace mmkv

#endif // MMKV_CONFIGD_SERVER_H_
