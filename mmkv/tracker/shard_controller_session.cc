// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#include "shard_controller_session_impl.h"

#include "mmkv/tracker/shard_controller_codec.h"

ShardControllerSession::ShardControllerSession() {}

ShardControllerSession::~ShardControllerSession() noexcept {}

void ShardControllerSession::AddNode(
    ShardControllerServer *server,
    TcpConnection         *conn,
    ShardControllerCodec  *codec,
    ControllerRequest     &req
)
{
  LOG_TRACE << "New node join to the cluster";

  /* If node isn't added or cluster is full, reject the node.
     The cluster can hold (shard number) nodes at most since
     the node must hold one shard at least. */
  if (INVALID_NODE_ID != req.node_id()) {
    auto &node_conf_map  = server->GetRecentConf()->node_conf_map();
    auto  node_conf_iter = node_conf_map.find(req.node_id());
    if (node_conf_map.end() != node_conf_iter) {
      LOG_DEBUG << "The node has added: " << req.node_id();
      Impl::SendRejectResponse(codec, conn, CONTROL_STATUS_NODE_HAS_ADDED);
    } else {
      LOG_DEBUG << "This is a node with unknown id";
    }
    conn->ShutdownWrite();
    return;
  }

  MutexGuard guard(server->pending_conf_lock_);

  auto *p_conf = server->GetRecentConf();
  if ((size_t)p_conf->node_conf_map_size() + 1 > server->GetShardNum()) {
    LOG_DEBUG << "The cluster is full, can't add new node";
    Impl::SendRejectResponse(codec, conn, CONTROL_STATUS_NODE_FULL);
    conn->ShutdownWrite();
    return;
  }

  LOG_DEBUG << "Sharder port = " << req.sharder_port();

  ControllerResponse response;

  const auto peer_node_id = server->GenerateNodeId();
  response.set_node_id(peer_node_id);

  auto node_num = p_conf->node_conf_map_size();
  ++node_num;

  assert(node_num > 0);

  // The node holds shard num at least
  auto every_node_at_least = server->shard_num_ / node_num;
  // Some node can holds (every_node_at_least + 1)
  auto more_than_one_num   = server->shard_num_ % node_num;

  LOG_DEBUG << "Every node should hold " << every_node_at_least << " at least";
  LOG_DEBUG << "Allow " << more_than_one_num << " nodes can hold more than 1 shard";

  // Prepare the new pending configuration
  PendingConf new_pending_conf;
  new_pending_conf.conf = *p_conf; // copy from old conf

  new_pending_conf.node_id = peer_node_id;
  new_pending_conf.state   = CONF_STATE_ADD_NODE;

  // Prepare the new configuration
  NodeConf new_node_conf;
  new_node_conf.set_host(conn->GetPeerAddr().ToIp());
  new_node_conf.set_port(req.sharder_port());

  // The first node can't steal any shard from other nodes
  // but return all shards to the first node

  /* Steal shards from other nodes */
  auto *p_new_conf_shard_ids = new_node_conf.mutable_shard_ids();

  /* node_infos[{node_id, host, port, is_push, shard_ids[]}] */
  auto *p_resp_node_infos = response.mutable_node_infos();
  if (1 == node_num) {
    LOG_DEBUG << "This is the first node of cluster";
    // The first node hold all shards
    p_new_conf_shard_ids->Reserve(server->shard_num_);

    response.set_shard_num(server->GetShardNum());
    // auto *p_new_node_info = p_resp_node_infos->Add();
    // Set the node_id to itself since no other nodes can be stealed.
    // p_new_node_info->set_node_id(peer_node_id);
    // p_new_node_info->set_port(req.sharder_port());
    // p_new_node_info->set_host(new_node_conf.host());
    // auto *p_new_node_info_shard_ids = p_new_node_info->mutable_shard_ids();
    // p_new_node_info_shard_ids->Reserve(server->GetShardNum());

    for (uint32_t i = 0; i < server->shard_num_; ++i) {
      p_new_conf_shard_ids->AddAlreadyReserved(i);
      // p_new_node_info_shard_ids->AddAlreadyReserved(i);
    }

    // Tell the peer with its node id
    response.set_node_id(peer_node_id);
    response.set_status(CONTROL_STATUS_OK);
  } else {
    LOG_DEBUG << "Node num > 1, Redistribute the shards";

    // Use new pending conf's map instead old conf,
    // we need modify the old conf in pending conf.
    auto *node_conf_map = new_pending_conf.conf.mutable_node_conf_map();
    for (auto &node_id_conf : *node_conf_map) {
      auto *p_new_node_info = p_resp_node_infos->Add();

      const auto node_id    = node_id_conf.first;
      auto      &node_conf  = node_id_conf.second;
      const auto shard_size = node_conf.shard_ids_size();
      int64_t    diff       = shard_size - every_node_at_least;
      LOG_DEBUG << "Node id = " << node_id;
      LOG_DEBUG << "Shard size = " << shard_size;
      LOG_DEBUG << "Move node num = " << diff;
      if (more_than_one_num > 0) {
        diff--;
        more_than_one_num--;
      }
      if (diff <= 0) continue;

      const int64_t end = shard_size - diff;

      p_new_node_info->set_node_id(node_id);
      p_new_node_info->set_host(node_conf.host());
      p_new_node_info->set_port(node_conf.port());
      auto *p_new_node_info_shard_ids = p_new_node_info->mutable_shard_ids();
      p_new_node_info_shard_ids->Reserve(diff);

      auto *p_old_node_conf_shard_ids = node_conf.mutable_shard_ids();
      for (int64_t i = shard_size - 1; i >= end; --i) {
        const auto old_shard_id =
            p_old_node_conf_shard_ids->Get(p_old_node_conf_shard_ids->size() - 1);
        p_new_conf_shard_ids->AddAlreadyReserved(old_shard_id);
        p_new_node_info_shard_ids->AddAlreadyReserved(old_shard_id);
        p_old_node_conf_shard_ids->RemoveLast();
      }
      // FIXME Need shrink?
      // p_old_node_conf_shard_ids->Truncate(p_old_node_conf_shard_ids->size());
      p_new_node_info->set_is_push(false);
    }
    response.set_status(CONTROL_STATUS_OK);
  }

  new_pending_conf.conf.mutable_node_conf_map()->emplace(peer_node_id, std::move(new_node_conf));
  LOG_TRACE << "(Join)Total node number=" << new_pending_conf.conf.node_conf_map_size();
  server->PushPendingConf(&new_pending_conf);

  /* Record the node_id
   * to make after request
   * no need store the node_id
   */
  node_id_ = req.node_id();
  // server->node_session_map[node_id_] = this;

  codec->Send(conn, &response);
}

void ShardControllerSession::Leave(
    ShardControllerServer *server,
    TcpConnection         *conn,
    ShardControllerCodec  *codec,
    ControllerRequest     &req
)
{
  LOG_TRACE << "A node is leaving";

  if (INVALID_NODE_ID == req.node_id()) {
    LOG_DEBUG << "The node isn't added, can't leave from this cluster";
    Impl::SendRejectResponse(codec, conn, CONTROL_STATUS_NODE_NON_EXISTS);
    return;
  }

  const auto peer_node_id   = req.node_id();
  auto      *p_conf         = server->GetRecentConf();
  auto      &node_conf_map  = p_conf->node_conf_map();
  auto       node_conf_iter = node_conf_map.find(peer_node_id);

  if (node_conf_map.end() == node_conf_iter) {
    LOG_DEBUG << "The node isn't added, can't leave from this cluster";
    Impl::SendRejectResponse(codec, conn, CONTROL_STATUS_NODE_NON_EXISTS);
    return;
  }
  assert(node_id_ == peer_node_id);

  auto &node_conf     = node_conf_iter->second;
  auto &remove_shards = node_conf.shard_ids();
  u64   removed_index = 0;
  u64   shard_num     = remove_shards.size();
  u64   new_node_num  = p_conf->node_conf_map_size() - 1;

  assert(p_conf->node_conf_map_size() >= 1);
  /* Leave我认为有两种方案
   * 1）将leaved节点的shards尽可能分发到各个节点
   * 但是需要其他节点与该shard建立连接，
   * 并发送GetShard请求获取分片
   * 2）leaved节点发送PutShard请求将分片推送给其他节点
   *
   * 这里我采用方案2
   *
   * 其实就是一个硬币的两面
   */

  PendingConf pending_conf;
  pending_conf.conf    = *p_conf;
  pending_conf.node_id = peer_node_id;
  pending_conf.state   = CONF_STATE_LEAVE_NODE;

  ControllerResponse resp;
  resp.set_node_id(peer_node_id);
  auto *p_new_node_infos = resp.mutable_node_infos()->Add();

  if (new_node_num > 0) {
    // Get the old node shard conf and remove it from pending conf
    // The old conf is readonly, don't modify it!
    // auto  new_node_conf_iter = pending_conf.conf.mutable_node_conf_map()->find(peer_node_id);
    // auto  old_node_conf      = std::move(*new_node_conf_iter);
    auto *p_pending_conf_map = pending_conf.conf.mutable_node_conf_map();
    p_pending_conf_map->erase(peer_node_id);

    const u64 at_least_shard_num = shard_num / new_node_num;
    u64       more_than_node_num = shard_num % new_node_num;

    for (auto &node_id_conf : *p_pending_conf_map) {
      auto *p_new_node_info_shard_ids = p_new_node_infos->mutable_shard_ids();

      const auto node_id    = node_id_conf.first;
      auto      &node_conf  = node_id_conf.second;
      const auto shard_size = node_conf_map.size();
      int64_t    diff       = at_least_shard_num - shard_size;
      if (more_than_node_num > 0) {
        more_than_node_num--;
        diff++;
      }
      if (diff <= 0) continue;

      p_new_node_info_shard_ids->Reserve(diff);
      auto *p_old_node_conf_shard_ids = node_conf.mutable_shard_ids();
      p_old_node_conf_shard_ids->Reserve(p_old_node_conf_shard_ids->size() + diff);
      for (int64_t i = 0; i < diff; ++i) {
        if ((size_t)remove_shards.size() != removed_index) {
          const auto old_node_id = remove_shards[removed_index++];
          p_old_node_conf_shard_ids->AddAlreadyReserved(old_node_id);
          p_new_node_info_shard_ids->AddAlreadyReserved(old_node_id);
        } else
          break;
      }

      p_new_node_infos->set_node_id(node_id);
      p_new_node_infos->set_host(node_conf.host());
      p_new_node_infos->set_is_push(true);
      if ((size_t)remove_shards.size() == removed_index) break;
    }
  }

  // Remove the old node and push new conf
  LOG_TRACE << "(Leave)Total node number=" << pending_conf.conf.node_conf_map_size();
  server->PushPendingConf(&pending_conf);
  server->node_session_map.erase(peer_node_id);

  resp.set_status(CONTROL_STATUS_OK);

  codec->Send(conn, &resp);
}

void ShardControllerSession::AddNodeComplete(
    ShardControllerServer  *server,
    TcpConnectionPtr const &conn,
    ShardControllerCodec   *codec,
    ControllerRequest      &req
)
{
  Impl::ControllorOperationComplete(this, server, conn, codec, req, CONF_STATE_ADD_NODE);
}

void ShardControllerSession::LeaveNodeComplete(
    ShardControllerServer  *server,
    TcpConnectionPtr const &conn,
    ShardControllerCodec   *codec,
    ControllerRequest      &req
)
{
  Impl::ControllorOperationComplete(this, server, conn, codec, req, CONF_STATE_LEAVE_NODE);
}
