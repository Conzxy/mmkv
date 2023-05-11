// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#include "shard_controller_server.h"
#include "shard_controller_session.h"

#include "controller.pb.h"
#include "mmkv/configd/configd.h"
#include "mmkv/configd/configd_config.h"

#include <random>

using namespace mmkv::server;
using namespace kanon;

ShardControllerServer::ShardControllerServer(EventLoop *loop, InetAddr const &addr)
  : server_(loop, addr, "ShardController")
  , codec_()
  , shard_num_(configd_config().shard_num)
{
  codec_.SetMessageCallback(
      [this](TcpConnectionPtr const &conn, Buffer &buffer, size_t payload_size, TimeStamp) {
        ControllerRequest req;
        protobuf::ParseFromBuffer(&req, payload_size, &buffer);

        auto *p_session = AnyCast2<ShardControllerSession *>(conn->GetContext());

        LOG_DEBUG << req.DebugString();
        switch (req.operation()) {
          case CONTROL_OP_ADD_NODE:
            p_session->Join(this, conn.get(), &codec_, req);
            break;
          case CONTROL_OP_LEAVE_NODE:
            p_session->Leave(this, conn.get(), &codec_, req);
            break;
          case CONTROL_OP_ADD_NODE_COMPLETE:
            p_session->JoinComplete(this, conn, &codec_, req);
            break;
          case CONTROL_OP_LEAVE_NODE_COMPLETE:
            p_session->LeaveComplete(this, conn, &codec_, req);
            break;
          case CONTROL_OP_QUERY_NODE_INFO:
            p_session->QueryNodeInfo(this, conn, &codec_, req);
          default:;
            conn->ShutdownWrite();
        }
      }
  );

  server_.SetConnectionCallback([this](TcpConnectionPtr const &conn) {
    if (conn->IsConnected()) {
      auto p_session = new ShardControllerSession();
      conn->SetContext(*p_session);
      codec_.SetUpConnection(conn);
    } else {
      auto p_session = AnyCast<ShardControllerSession>(conn->GetContext());
      node_session_map.erase(p_session->node_id_);
      delete p_session;
    }
  });
}

u64 ShardControllerServer::GenerateNodeId() const
{
  static std::random_device                 rd{};
  // The -1 as the non-joined node id
  static std::uniform_int_distribution<u64> uid(0, std::numeric_limits<u64>::max() - 1);
  static std::default_random_engine         dre(rd());

  u64 id;
  u64 pre_id = -1;

  /* Use the recent pending configuration,
   * If the id from a removed node, this is also safe, since the pending state is
   * different. */
  auto const &node_conf_map = GetRecentConf()->node_conf_map();

  while (true) {
    id = uid(dre);
    LOG_DEBUG << "id = " << id;

    if (id == pre_id) continue;

    auto conf_node_iter = node_conf_map.find(id);

    if (node_conf_map.end() == conf_node_iter) {
      break;
    }

    pre_id = id;
  }

  return id;
}

void ShardControllerServer::CheckPendingConfSessionAndResponse()
{
  ControllerResponse resp;
  resp.set_status(CONTROL_STATUS_CONF_CHANGE);

  while (true) {
    auto         p_recent_conf = GetRecentPendingConf();
    PendingState pending_state;
    pending_state.state   = p_recent_conf->state;
    pending_state.node_id = p_recent_conf->node_id;

    auto p_wconn_slot = pending_conf_conn_dict_.FindSlot(pending_state);

    if (!p_wconn_slot) {
      break;
    }

    auto p_conn = (*p_wconn_slot)->value.value.lock();
    if (p_conn) {
      codec_.Send(p_conn.get(), &resp);
      UpdateConfig(std::move(p_recent_conf->conf));
    } else {
      // Connection is down
      pending_conf_conn_dict_.EraseAfterFindSlot(p_wconn_slot);

      // FIXME allocate the shards to other nodes.
    }

    PopPendingConf();
  }
}

void ShardControllerServer::UpdateConfig(Configuration &&conf)
{
  config_ = std::move(conf);

  LOG_DEBUG << "Recent config: " << config_.DebugString();
  p_configd_->SyncConfig(config_);
}
