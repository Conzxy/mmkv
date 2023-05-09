// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#include "shard_controller_session_impl.h"

#include "mmkv/tracker/shard_controller_codec.h"

ShardControllerSession::ShardControllerSession() {}

ShardControllerSession::~ShardControllerSession() noexcept {}

void ShardControllerSession::Join(
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

  auto *p_recent_conf = server->GetRecentConf();
  if ((size_t)p_recent_conf->node_conf_map_size() + 1 > server->GetShardNum()) {
    LOG_DEBUG << "The cluster is full, can't add new node";
    Impl::SendRejectResponse(codec, conn, CONTROL_STATUS_NODE_FULL);
    conn->ShutdownWrite();
    return;
  }

  LOG_DEBUG << "Sharder port = " << req.sharder_port();

  ControllerResponse response;

  const auto peer_node_id = server->GenerateNodeId();
  response.set_node_id(peer_node_id);

  auto new_node_num = p_recent_conf->node_conf_map_size() + 1;
  assert(new_node_num > 0);

  // The node holds shard num at least
  auto every_node_at_least = server->shard_num_ / new_node_num;
  // Some node can holds (every_node_at_least + 1)
  auto more_than_one_num   = server->shard_num_ % new_node_num;

  LOG_DEBUG << "Every node should hold " << every_node_at_least << " at least";
  LOG_DEBUG << "Allow " << more_than_one_num << " nodes can hold more than 1 shard";

  // Prepare the new pending configuration
  PendingConf new_pending_conf;
  new_pending_conf.conf = *p_recent_conf; // copy from old conf

  new_pending_conf.node_id = peer_node_id;
  new_pending_conf.state   = CONF_STATE_JOIN_NODE;

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
  if (1 == new_node_num) {
    LOG_DEBUG << "This is the first node of cluster";

    // Don't send any node infos to peer to indicates
    // it is the first node of the cluster

    // Tell the peer with its node id
    response.set_node_id(peer_node_id);
    response.set_shard_num(server->GetShardNum());
    response.set_status(CONTROL_STATUS_OK);
  } else {
    LOG_DEBUG << "Node num > 1, Redistribute the shards";

    // Use new pending conf's map instead recent conf,
    // we need modify the old conf in pending conf.
    auto *pending_node_conf_map = new_pending_conf.conf.mutable_node_conf_map();
    for (auto &pending_node_id_conf : *pending_node_conf_map) {
      // The new pending conf holds the recent conf, this is also recent info
      const auto pending_node_id    = pending_node_id_conf.first;
      auto      &pending_node_conf  = pending_node_id_conf.second;
      const auto pending_shard_size = pending_node_conf.shard_ids_size();
      int64_t    diff               = pending_shard_size - every_node_at_least;
      LOG_DEBUG << "Node id = " << pending_node_id;
      LOG_DEBUG << "Shard size = " << pending_shard_size;
      LOG_DEBUG << "Move node num = " << diff;

      // To some cases, the shard of old node is reach the at least num,
      // the diff == 0, we can't pull any shard from it.
      assert(diff >= 0);
      if (more_than_one_num > 0) {
        if (diff > 0) {
          diff--;
          more_than_one_num--;
        } else {
          continue;
        }
      }

      // We don't known the node infos size, so I can't reserve it.
      auto *p_resp_node_info = p_resp_node_infos->Add();

      const int64_t end = pending_shard_size - diff;

      auto *p_resp_node_info_shard_ids = p_resp_node_info->mutable_shard_ids();
      p_resp_node_info_shard_ids->Reserve(diff);

      auto *p_pending_node_conf_shard_ids = pending_node_conf.mutable_shard_ids();
      for (int64_t i = pending_shard_size - 1; i >= end; --i) {
        const auto old_shard_id = p_pending_node_conf_shard_ids->Get(i);

        // Update the conf of new node
        p_new_conf_shard_ids->AddAlreadyReserved(old_shard_id);
        p_resp_node_info_shard_ids->AddAlreadyReserved(old_shard_id);
      }
      p_pending_node_conf_shard_ids->Truncate(p_pending_node_conf_shard_ids->size() - diff);

      // FIXME Need shrink?
      // p_old_node_conf_shard_ids->Truncate(p_old_node_conf_shard_ids->size());
      p_resp_node_info->set_node_id(pending_node_id);
      p_resp_node_info->set_host(pending_node_conf.host());
      p_resp_node_info->set_port(pending_node_conf.port());
      p_resp_node_info->set_is_push(false);
      LOG_DEBUG << "Pull following shards to node [" << pending_node_id << ", "
                << pending_node_conf.host() << ":" << pending_node_conf.port() << "]";
      for (auto const shard_id : p_resp_node_info->shard_ids()) {
        LOG_DEBUG << "node id = " << shard_id;
      }
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

  const auto peer_node_id          = req.node_id();
  auto      *p_recent_conf         = server->GetRecentConf();
  auto      &recent_node_conf_map  = p_recent_conf->node_conf_map();
  auto       recent_node_conf_iter = recent_node_conf_map.find(peer_node_id);

  if (recent_node_conf_map.end() == recent_node_conf_iter) {
    LOG_DEBUG << "The node isn't joined, can't leave from this cluster";
    Impl::SendRejectResponse(codec, conn, CONTROL_STATUS_NODE_NON_EXISTS);
    return;
  }

  assert(node_id_ == peer_node_id);
  LOG_DEBUG << "The node: [" << node_id_ << "] is leaving";

  auto     &recent_node_conf  = recent_node_conf_iter->second;
  auto     &remove_shards     = recent_node_conf.shard_ids();
  u64       removed_index     = 0;
  const u64 removed_shard_num = remove_shards.size();
  const u64 new_node_num      = p_recent_conf->node_conf_map_size() - 1;

  assert(p_recent_conf->node_conf_map_size() >= 1);
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

  // We can't modify the recent configuration(from config_ or pending config)
  pending_conf.conf    = *p_recent_conf;
  pending_conf.node_id = peer_node_id;
  pending_conf.state   = CONF_STATE_LEAVE_NODE;

  ControllerResponse resp;
  resp.set_node_id(peer_node_id);

  if (new_node_num > 0) {
    // Get the old node shard conf and remove it from pending conf
    // The old conf is readonly, don't modify it!
    // auto  new_node_conf_iter = pending_conf.conf.mutable_node_conf_map()->find(peer_node_id);
    // auto  old_node_conf      = std::move(*new_node_conf_iter);
    auto *p_pending_conf_map = pending_conf.conf.mutable_node_conf_map();
    p_pending_conf_map->erase(peer_node_id);

    const u64 at_least_shard_num = removed_shard_num / new_node_num;
    u64       more_than_node_num = removed_shard_num % new_node_num;

    for (auto &pending_node_id_conf : *p_pending_conf_map) {
      const auto pending_node_id   = pending_node_id_conf.first;
      auto      &pending_node_conf = pending_node_id_conf.second;

      const auto pending_shard_size = pending_node_conf.shard_ids_size();
      assert((u64)pending_shard_size < at_least_shard_num);
      int64_t diff = at_least_shard_num - pending_shard_size;

      assert(diff >= 0);
      if (more_than_node_num > 0) {
        if (diff == 0) {
          continue;
        } else {
          more_than_node_num--;
          diff++;
        }
      }

      auto *p_resp_node_info           = resp.mutable_node_infos()->Add();
      auto *p_resp_node_info_shard_ids = p_resp_node_info->mutable_shard_ids();
      assert(p_resp_node_info_shard_ids->size() == 0);
      auto *p_pending_node_conf_shard_ids = pending_node_conf.mutable_shard_ids();

      p_resp_node_info_shard_ids->Reserve(diff);
      p_pending_node_conf_shard_ids->Reserve(p_pending_node_conf_shard_ids->size() + diff);
      for (int64_t i = 0; i < diff; ++i) {
        if ((size_t)remove_shards.size() != removed_index) {
          const auto old_node_id = remove_shards[removed_index++];

          // Update the pending node conf(old node)
          p_pending_node_conf_shard_ids->AddAlreadyReserved(old_node_id);
          p_resp_node_info_shard_ids->AddAlreadyReserved(old_node_id);
        } else
          break;
      }

      p_resp_node_info->set_node_id(pending_node_id);
      p_resp_node_info->set_host(pending_node_conf.host());
      p_resp_node_info->set_port(pending_node_conf.port());
      p_resp_node_info->set_is_push(true);

      LOG_DEBUG << "Push following shards to node [" << pending_node_id << ", "
                << pending_node_conf.host() << ":" << pending_node_conf.port() << "]";
      for (auto const shard_id : p_resp_node_info->shard_ids()) {
        LOG_DEBUG << "node id = " << shard_id;
      }

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

void ShardControllerSession::JoinComplete(
    ShardControllerServer  *server,
    TcpConnectionPtr const &conn,
    ShardControllerCodec   *codec,
    ControllerRequest      &req
)
{
  Impl::ControllorOperationComplete(this, server, conn, codec, req, CONF_STATE_JOIN_NODE);
}

void ShardControllerSession::LeaveComplete(
    ShardControllerServer  *server,
    TcpConnectionPtr const &conn,
    ShardControllerCodec   *codec,
    ControllerRequest      &req
)
{
  Impl::ControllorOperationComplete(this, server, conn, codec, req, CONF_STATE_LEAVE_NODE);
}
