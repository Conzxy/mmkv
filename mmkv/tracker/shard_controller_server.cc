// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#include "shard_controller_server.h"
#include "shard_controller_session.h"

#include "controller.pb.h"
#include <random>

using namespace mmkv::server;
using namespace kanon;

ShardControllerServer::ShardControllerServer(EventLoop *loop, InetAddr const &addr)
  : server_(loop, addr, "ShardController")
  , codec_(SHARDER_CONTROLLER_TAG, SHARDER_CONTROLLER_MAX_SIZE)
  , shard_num_()
{
  codec_.SetMessageCallback(
      [this](TcpConnectionPtr const &conn, Buffer &buffer, size_t payload_size, TimeStamp) {
        ControllerRequest req;
        protobuf::ParseFromBuffer(&req, payload_size, &buffer);

        auto *p_session = AnyCast2<ShardControllerSession *>(conn->GetContext());
        switch (req.operation()) {
          case CONTROL_OP_ADD_NODE:
            p_session->AddNode(this, conn.get(), &codec_, req);
            break;
          case CONTROL_OP_LEAVE_NODE:
            p_session->Leave(this, conn.get(), &codec_, req);
            break;
          case CONTROL_OP_ADD_NODE_COMPLETE:
            p_session->AddNodeComplete(this, conn, &codec_, req);
            break;
          case CONTROL_OP_LEAVE_NODE_COMPLETE:
            p_session->LeaveNodeComplete(this, conn, &codec_, req);
            break;
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
  static std::uniform_int_distribution<u64> uid(0);
  static std::default_random_engine         dre(rd());

  u64 id;
  u64 pre_id = -1;

  /* Use the recent pending configuration,
   * If the id from a removed node, this is also safe, since the pending state is
   * different. */
  auto const &node_conf_map = GetRecentConf()->node_conf_map();

  while (true) {
    id = uid(dre);

    if (id == pre_id) continue;

    auto conf_node_iter = node_conf_map.find(id);

    if (node_conf_map.end() != conf_node_iter) {
      break;
    }

    pre_id = conf_node_iter->first;
  }

  return id;
}

void ShardControllerServer::CheckPendingConfSessionAndResponse()
{
  ControllerResponse resp;
  resp.set_status(CONTROL_STATUS_CONF_CHANGE);

  while (true) {
    auto         recent_conf = GetRecentPendingConf();
    PendingState pending_state;
    pending_state.state   = recent_conf.state;
    pending_state.node_id = recent_conf.node_id;

    auto p_wconn_slot = pending_conf_conn_dict_.FindSlot(pending_state);

    if (!p_wconn_slot) {
      break;
    }

    auto p_conn = (*p_wconn_slot)->value.value.lock();
    if (p_conn) {
      codec_.Send(p_conn.get(), &resp);
      config_ = std::move(recent_conf.conf);
    } else {
      // Connection is down
      pending_conf_conn_dict_.EraseAfterFindSlot(p_wconn_slot);

      // FIXME allocate the shards to other nodes.
    }

    PopPendingConf();
  }
}