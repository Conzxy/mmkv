// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#ifndef _MMKV_SERVER_MMKV_SERVER_H_
#define _MMKV_SERVER_MMKV_SERVER_H_

#include "kanon/util/noncopyable.h"
#include "kanon/net/user_server.h"
#include "mmkv/protocol/mmbp_codec.h"
#include "mmkv/tracker/shard_controller_client.h"

namespace mmkv {
namespace server {

class MmkvSession;

class MmkvServer {
  DISABLE_EVIL_COPYABLE(MmkvServer)
  friend class MmkvSession;

  using Codec = protocol::MmbpCodec;

 public:
  explicit MmkvServer(EventLoop *loop, InetAddr const &addr, InetAddr const &sharder_addr);
  ~MmkvServer() noexcept;

  void Listen() { server_.StartRun(); }

  void SetLoopNum(int num) { server_.SetLoopNum(num); }

  void Start();

 private:
  TcpServer server_;

  Codec codec_;

  // std::unique_ptr<EventLoopThread> tracker_cli_loop_thr_;
  std::unique_ptr<ShardControllerClient> ctler_cli_;
};

} // namespace server
} // namespace mmkv

#endif // _MMKV_SERVER_MMKV_SERVER_H_
