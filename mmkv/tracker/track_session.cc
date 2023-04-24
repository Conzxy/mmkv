// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#include "track_session.h"

#include "mmkv/protocol/track_request.h"
#include "mmkv/protocol/track_response.h"

#include "tracker.h"

using namespace mmkv::server;
using namespace mmkv::protocol;
using namespace kanon;
using namespace std::placeholders;

TrackSession::TrackSession(Tracker *tracker, TcpConnectionPtr const &conn)
  : tracker_(tracker)
  , codec_(TrackRequest::prototype, conn)
  , conn_(conn.get())
{
  codec_.SetMessageCallback(
      std::bind(&TrackSession::OnMessage, this, _1, _2, _3, _4));
}

void TrackSession::OnMessage(TcpConnectionPtr const &conn, Buffer &buffer,
                             uint32_t, TimeStamp recv_tm)
{
  TrackRequest req;
  req.ParseFrom(buffer);

  switch (req.GetOperation()) {
  case TO_ADD_NODE:
    AddNode(req);
    break;
  case TO_MOVE_SHARD:
    MoveShard(req);
    break;

  case TO_MOVE_FINISH: {
    tracker_->state_ = Tracker::IDLE;
  } break;

  case TO_LEAVE: {
    TrackResponse response;
    Leave(req, &response);
    codec_.Send(conn_, &response);
  } break;

  case TO_QUERY:
    Query(req);
    break;
  }
}

void TrackSession::AddNode(TrackRequest &req)
{
  LOG_TRACE << "New node join to the cluster";
  LOG_TRACE << "Node ID=" << tracker_->node_id_;
  LOG_TRACE << "Sharder port=" << req.sharder_port;

  TrackResponse response;
  response.node_id = tracker_->node_id_;
  response.SetNodeId();
  if (tracker_->state_ == Tracker::BUSY) {
    response.status_code = TS_WAIT;
  } else {
    response.status_code = TS_ADD_NODE_OK;
    auto node_num = tracker_->node_id_ + 1;
    // The node holds shard num at least
    auto every_node_at_least = tracker_->shards_.size() / node_num;
    // Some node can holds (every_node_at_least + 1)
    auto more_than_one_num = tracker_->shards_.size() % node_num;

    LOG_DEBUG << "every node at least = " << every_node_at_least;
    LOG_DEBUG << "more_than_one_num = " << more_than_one_num;

    // The first node can't steal any shard from other nodes
    // but return all shards to the first node
    if (node_num == 1) {
      std::vector<uint32_t> shards(tracker_->shards_.size());
      for (uint32_t i = 0; i < tracker_->shards_.size(); ++i)
        shards[i] = i;
      tracker_->node_map_.emplace(tracker_->node_id_,
                                  Tracker::ShardAndAddress{});
      response.shard_2d.emplace_back(std::move(shards));
    } else {
      LOG_DEBUG << "Node num > 1";
      for (auto &node_value : tracker_->node_map_) {
        auto &shard_addr = node_value.second;
        int32_t shard_size = shard_addr.shards.size();
        LOG_DEBUG << "Node id = " << node_value.first;
        LOG_DEBUG << "Shard size = " << shard_size;
        int32_t diff = shard_size - every_node_at_least;
        LOG_DEBUG << "Move node num = " << diff;
        if (more_than_one_num > 0) {
          diff--;
          more_than_one_num--;
        }
        if (diff <= 0) continue;
        Tracker::Shards shards;

        const int32_t end = shard_size - diff;
        for (int32_t i = shard_size - 1; i >= end; --i) {
          shards.emplace_back(shard_addr.shards[i]);
        }
        response.addrs.emplace_back(shard_addr.address);
        response.ports.emplace_back(shard_addr.port);
        response.shard_2d.emplace_back(std::move(shards));
      }
    }

    auto &node_mapped = tracker_->node_map_[response.node_id];
    node_mapped.address = conn_->GetPeerAddr().ToIp();
    node_mapped.port = req.sharder_port;
    response.SetAddrs();
    response.SetShard2D();
    tracker_->node_id_++;
    if (!response.shard_2d.empty()) tracker_->state_ = Tracker::BUSY;
    tracker_->node_num_++;
    /* Record the node_id
     * to make after request
     * no need store the node_id
     */
    node_id_ = tracker_->node_id_;
    tracker_->node_session_map[node_id_] = this;
    LOG_TRACE << "(Join)Total node number=" << tracker_->node_num_;
  }

  codec_.Send(conn_, &response);
}

#define LEAVE_DISPATCH 0
void TrackSession::Leave(TrackRequest &req, TrackResponse *response)
{
  auto node_map_iter = tracker_->node_map_.find(node_id_);
  assert(node_map_iter != tracker_->node_map_.end());
  auto &node_mapped = node_map_iter->second;
  auto &remove_shards = node_mapped.shards;
  size_t removed_index = 0;
  uint32_t shard_num = node_mapped.shards.size();
  uint32_t new_node_num = tracker_->node_num_ - 1;
  /* Leave我认为有两种方案
   * 1）将leaved节点的shards尽可能分发到各个节点
   * 但是需要其他节点与该shard建立连接，
   * 并发送GetShard请求获取分片
   * 2）leaved节点发送PutShard请求将分片推送给其他节点
   *
   * 这里我采用方案2
   */
#if LEAVE_DISPATCH
  std::vector<Tracker::Node> dispatch_nodes;
  std::vector<Tracker::Shards> dispatch_shards;
#endif

  if (tracker_->state_ != Tracker::BUSY) {
    if (new_node_num == 0) {
      tracker_->node_map_.clear();
      tracker_->node_num_--;
    } else {
      uint32_t at_least_shard_num = shard_num / new_node_num;
      uint32_t more_than_node_num = shard_num % new_node_num;

      for (auto &node_m : tracker_->node_map_) {
        auto &mapped = node_m.second;
        auto shard_size = mapped.shards.size();
        int32_t diff = at_least_shard_num - shard_size;
        if (more_than_node_num > 0) {
          more_than_node_num--;
          diff++;
        }
        if (diff <= 0) continue;

        std::vector<Shard> shards;
        for (int32_t i = 0; i < diff; ++i) {
          /* 注意，不要改动remove_shards原件
           * 因为此时整个Leave操作并未完成
           * 不能改变shard到node的映射关系
           */
          if (removed_index != remove_shards.size()) {
            shards.emplace_back(remove_shards[removed_index++]);
          } else
            break;
        }

        if (remove_shards.size() == removed_index) break;
#if LEAVE_DISPATCH
        dispatch_nodes.push_back(node_m.first);
        dispatch_shards.emplace_back(std::move(shards));
#endif
        response->addrs.emplace_back(node_mapped.address);
        response->ports.push_back(node_mapped.port);
        response->shard_2d.emplace_back(std::move(shards));
      }

#if LEAVE_DISPATCH
      // 分发给其他节点来分割该节点的分片
      for (size_t i = 0; i < dispatch_nodes.size(); ++i) {
        auto node = dispatch_nodes[i];
        auto session = tracker_->node_session_map[node];
        TrackResponse response;
        response.status_code = TS_MOVE_LEAVED_NODE;
        response.addrs.emplace_back(node_mapped.address);
        response.ports.push_back(node_mapped.port);
        response.shard_2d.emplace_back(dispatch_nodes[i]);
        response.SetAddrs();
        response.SetShard2D();
        session->codec_.Send(session->conn_, &response);
      }
#endif

      response->status_code = TS_LEAVE_OK;
      response->SetAddrs();
      response->SetShard2D();
      tracker_->node_num_--;
      tracker_->node_map_.erase(node_map_iter);
      LOG_TRACE << "(Leave)Total node number=" << tracker_->node_num_;
    }
  } else {
    response->status_code = TS_WAIT;
  }
}

void TrackSession::Query(TrackRequest &req) {
  TrackResponse response;
  response.status_code = TS_QUERY_OK;
  for (auto const &node_and_shard_addr : tracker_->node_map_) {
    response.nodes.emplace_back(node_and_shard_addr.first);
    auto &mapped = node_and_shard_addr.second;
    response.addrs.emplace_back(mapped.address);
    response.ports.emplace_back(mapped.port);
    response.shard_2d.emplace_back(mapped.shards);
  }

  response.SetAddrs();
  response.SetShard2D();
  response.SetNodes();
  codec_.Send(conn_, &response);
}

void TrackSession::MoveShard(TrackRequest &req)
{
  if (req.HasNodeId() && req.HasShardId()) {
    uint32_t old_node_id = tracker_->shards_[req.shard_id];
    uint32_t old_shard = 0;
    // Not the first node
    if ((uint32_t)-1 != old_node_id) {
      auto &shards = tracker_->node_map_[old_node_id].shards;
      old_shard = shards.back();
      shards.pop_back(); MMKV_UNUSED(old_shard);
      assert(req.shard_id == old_shard);
      LOG_DEBUG << "Remove shard from " << old_node_id;
    } else {
      LOG_DEBUG << "Doesn't move shard from any node since "
                  "this request from the first node";
    }
    tracker_->shards_[req.shard_id] = req.node_id;
    tracker_->node_map_[req.node_id].shards.emplace_back(req.shard_id);
    LOG_DEBUG << "Removed shard id is " << req.shard_id;
  }
}
