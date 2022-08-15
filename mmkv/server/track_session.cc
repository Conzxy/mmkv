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
  codec_.SetMessageCallback(std::bind(&TrackSession::OnMessage, this, _1, _2, _3, _4));
}

void TrackSession::OnMessage(TcpConnectionPtr const &conn, 
  Buffer &buffer, uint32_t, TimeStamp recv_tm) {
  TrackRequest req;
  req.ParseFrom(buffer);

  switch (req.GetOperation()) {
  case TO_ADD_NODE:
    AddNode();
  break;
  case TO_MOVE_SHARD: {
    if (req.HasNodeId() && req.HasShardId()) {
      uint32_t old_node_id = tracker_->shards_[req.shard_id];
      if (req.node_id != old_node_id)
        tracker_->node_map[old_node_id].shards.pop_back();
      tracker_->shards_[req.shard_id] = req.node_id;
      tracker_->node_map[req.node_id].shards.emplace_back(req.shard_id);
    }
  }
  break;

  case TO_MOVE_FINISH: {
    tracker_->state_ = Tracker::IDLE;
  }
  break;
  }
}

void TrackSession::AddNode() {
  LOG_TRACE << "New node join to the cluster";
  LOG_TRACE << "Node ID=" << tracker_->node_id_;

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
      tracker_->node_map.emplace(tracker_->node_id_, Tracker::ShardAndAddress{});
      response.shard_2d.emplace_back(std::move(shards));
    } else {
      LOG_DEBUG << "Node num > 1";
      for (auto &node_value : tracker_->node_map) {
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
        for (int32_t i = shard_size - 1; i >= shard_size - diff; --i) {
          shards.emplace_back(shard_addr.shards[i]);
        }
        response.addrs.emplace_back(shard_addr.address);
        response.ports.emplace_back(shard_addr.port);
        response.shard_2d.emplace_back(std::move(shards));
      }
    }

    auto &node_mapped = tracker_->node_map[response.node_id];
    node_mapped.address = conn_->GetPeerAddr().ToIp();
    node_mapped.port = conn_->GetPeerAddr().GetPort();

    response.SetAddrs();
    response.SetShard2D();
    tracker_->node_id_++;
    tracker_->state_ = Tracker::BUSY;
    LOG_TRACE << "Total node number=" << tracker_->node_id_;
  }

  codec_.Send(conn_, &response);
}