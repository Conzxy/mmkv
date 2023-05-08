#ifndef _MMKV_SHARDER_FORWARDER_D_H__
#define _MMKV_SHARDER_FORWARDER_D_H__

#include <kanon/net/user_server.h>
#include "forwarder_codec.h"

namespace mmkv {
namespace server {

class ShardControllerClient;

class Forwarderd : kanon::noncopyable {
  using Codec = ForwarderCodec;

 public:
  Forwarderd(EventLoop *p_loop, InetAddr const &addr);
  ~Forwarderd() noexcept;

  void Listen();

 private:
  struct Impl;
  friend struct Impl;

  TcpServer server_;
  Codec     codec_;

  std::unique_ptr<ShardControllerClient> p_shard_ctl_cli_;
};

} // namespace server
} // namespace mmkv

#endif
