// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#include "tracker.h"

#include "mmkv/util/shard_util.h"
#include "track_session.h"
#include "mmkv/server/config.h"

using namespace mmkv::server;
using namespace mmkv::util;
using namespace kanon;

Tracker::Tracker(EventLoop *loop, InetAddr const &addr)
  : server_(loop, addr, "Tracker")
  , shards_(mmkv_config().shard_num, -1)
{
  server_.SetConnectionCallback([this](TcpConnectionPtr const &conn) {
    if (conn->IsConnected()) {
      auto session = new TrackSession(this, conn);
      /* node_session_map需在新节点显式请求加入节点后注册，
       * 即在TrackSession::Join()中处理 */
      conn->SetContext(*session);
    } else {
      auto session = AnyCast<TrackSession>(conn->GetContext());
      /* 无论存不存在，直接删除，因为判断有无也需要相同的时间复杂度 */
      node_session_map.erase(session->node_id_);
      RemoveNode(session->node_id_);
      delete session;
    }
  });
}

void Tracker::RemovePendingNodes()
{
  for (auto removed_node : removed_nodes_) {
    RemoveNode(removed_node);
  }
}

void Tracker::RemoveNode(Node node)
{
  if (state_ == BUSY) {
    server_.GetLoop()->RunAfter([this, node]() { RemoveNode(node); }, 0.5);
    return;
  }

  // Check if the node exists in the tracker
  auto node_map_iter = node_map_.find(node);
  if (node_map_iter == node_map_.end()) return;

  auto &node_mapped = node_map_iter->second;
  auto &remove_shards = node_mapped.shards;
  uint32_t shard_num = node_mapped.shards.size();
  uint32_t new_node_num = node_num_ - 1;

  if (new_node_num == 0) {
    node_map_.clear();
    node_num_--;
  } else {
    uint32_t at_least_shard_num = shard_num / new_node_num;
    uint32_t more_than_node_num = shard_num % new_node_num;

    for (auto &node_m : node_map_) {
      auto &mapped = node_m.second;
      auto shard_size = mapped.shards.size();
      int32_t diff = at_least_shard_num - shard_size;
      if (more_than_node_num > 0) {
        more_than_node_num--;
        diff++;
      }
      if (diff <= 0) continue;

      for (int32_t i = 0; i < diff; ++i) {
        if (!remove_shards.empty()) {
          mapped.shards.emplace_back(remove_shards.back());
          assert(shards_[remove_shards.back()] == node);
          shards_[remove_shards.back()] = node_m.first;
          remove_shards.pop_back();
        } else
          break;
      }

      if (remove_shards.empty()) break;
    }

    node_num_--;
  }

  node_map_.erase(node_map_iter);
  LOG_TRACE << "Total node number=" << node_num_;
}

void Tracker::QueryKeyMappedAddress(std::string const &key,
                                    std::string &address, uint16_t &port)
{
  auto shard_id = MakeShardId(key);
  auto node_id = shards_[shard_id];
  assert(node_map_.find(node_id) != node_map_.end());
  auto &node_mapped = node_map_[node_id];

  address = node_mapped.address;
  port = node_mapped.port;
}
