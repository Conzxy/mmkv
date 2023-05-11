// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#include "configd.h"

#include "configd.pb.h"

using namespace mmkv::server;
using namespace kanon;

Configd::Configd(EventLoop *loop, InetAddr const &addr, InetAddr const &ctler_addr)
  : server_(loop, addr, "ConfigServer")
  , ctler_loop_thr_("ctler")
  , ctler_(ctler_loop_thr_.StartRun(), ctler_addr)
{
  ctler_.Listen();
  ctler_.p_configd_ = this;

  codec_.SetMessageCallback(
      [this](TcpConnectionPtr const &conn, Buffer &buffer, size_t payload_size, TimeStamp) {
        ConfigRequest req;
        protobuf::ParseFromBuffer(&req, payload_size, &buffer);

        ConfigResponse resp;
        switch (req.operation()) {
          case CONF_OP_FETCH: {
            MutexGuard guard(ctler_.pending_conf_lock_);
            auto      *p_conf = ctler_.GetRecentConf();
            resp.set_status(CONF_STATUS_OK);
            *resp.mutable_conf()->mutable_node_conf_map() = p_conf->node_conf_map();
          } break;
          default: {
            resp.set_status(CONF_INVALID_REQ);
          } break;
        }

        codec_.Send(conn, &resp);
      }
  );

  server_.SetConnectionCallback([this](TcpConnectionPtr const &conn) {
    if (conn->IsConnected()) {
      codec_.SetUpConnection(conn);
    } else {
    }
  });

  LOG_INFO << "ConfigServer is constructed";
}

Configd::~Configd() noexcept { LOG_INFO << "ConfigServer is removed"; }

void Configd::SyncConfig(Configuration const &conf)
{
  auto conf_copy = conf.node_conf_map();

  ConfigResponse resp;

  resp.set_status(CONF_STATUS_OK);

  server_.ApplyAllPeers([this, &resp, &conf_copy](TcpConnectionPtr const &conn) {
    resp.mutable_conf()->mutable_node_conf_map()->swap(conf_copy);
    codec_.Send(conn, &resp);
    conf_copy.swap(*resp.mutable_conf()->mutable_node_conf_map());
  });
}
