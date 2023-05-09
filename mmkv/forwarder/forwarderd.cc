// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#include "forwarderd_impl.h"

Forwarderd::Forwarderd(EventLoop *p_loop, InetAddr const &addr)
  : server_(p_loop, addr, "Forwarder Deamon")
  , codec_()
{
  if (!mmkv_config().shard_controller_endpoint.empty()) {
    Impl::SetShardController(this, InetAddr(mmkv_config().shard_controller_endpoint));
  }

  codec_.SetMessageCallback(
      [this](TcpConnectionPtr const &conn, Buffer &buffer, size_t payload_size, TimeStamp) {
        ForwardRequest req;
        protobuf::ParseFromBuffer(&req, payload_size, &buffer);

        // FIXME check cli
        switch (req.operation()) {
          case FWD_MSG_JOIN: {
            assert(req.has_controller_host() && req.has_controller_port());

            InetAddr controller_addr(req.controller_host(), req.controller_port());
            Impl::SetShardController(this, controller_addr);
          } break;

          case FWD_MSG_LEAVE: {
            if (p_shard_ctl_cli_) {
              p_shard_ctl_cli_->Leave();
            } else {
            }
          } break;
        }
      }
  );

  server_.SetConnectionCallback([this](TcpConnectionPtr const &conn) {
    if (conn->IsConnected()) {
      codec_.SetUpConnection(conn);
    }
  });
}

Forwarderd::~Forwarderd() noexcept {}

void Forwarderd::Listen()
{
  LOG_INFO << "Start running forwarder deamon";
  server_.StartRun();
}
