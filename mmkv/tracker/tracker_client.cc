#include "tracker_client.h"

#include "mmkv/protocol/track_request.h"
#include "mmkv/protocol/track_response.h"

#include "mmkv/server/config.h"
#include "mmkv/sharder/shard_client.h"

#include <kanon/log/logger.h>

using namespace mmkv::server;
using namespace mmkv::protocol;
using namespace kanon;

TrackerClient::TrackerClient(EventLoop *loop, InetAddr const &addr,
                             InetAddr const &sharder_addr,
                             std::string const &name,
                             std::string const &sharder_name)
  : cli_(NewTcpClient(loop, addr, name))
  , codec_(TrackResponse::prototype)
  , shard_cli_loop_thr_("ShardClients")
  , sharder_loop_thr_(sharder_name)
  , sharder_(sharder_loop_thr_.StartRun(), sharder_addr) // default port:9998
  , sharder_port_(sharder_addr.GetPort())
{
  shard_cli_loop_thr_.StartRun();
  codec_.SetMessageCallback([this](TcpConnectionPtr const &conn, Buffer &buffer,
                                   uint32_t, TimeStamp recv_time) {
    TrackResponse response;
    response.ParseFrom(buffer);
    response.DebugPrint();

    switch (response.status_code) {
    case TS_ADD_NODE_OK: {
      if (response.HasAddrs() && response.HasShard2D() && response.HasNodeId())
      {
        addrs_ = std::move(response.addrs);
        ports_ = std::move(response.ports);
        if (addrs_.size() != ports_.size()) {
          // FIXME Error
          return;
        }

        shard_2d_ = std::move(response.shard_2d);
        node_id_ = response.node_id;
        if (IsTheFirstNode()) {
          SetAllShardNode();
        } else {
          auto const sharder_num = addrs_.size();
          LOG_DEBUG << "Sharder num=" << sharder_num;
          shard_clis_.reserve(sharder_num);
          for (size_t i = 0; i < sharder_num; ++i) {
            shard_clis_.emplace_back(shard_cli_loop_thr_.GetLoop(),
                                     InetAddr(addrs_[i], ports_[i]), this,
                                     shard_2d_[i]);
            shard_clis_[i].SetState(ShardClient::JOIN_TO);
            shard_clis_[i].Connect();
          }
        }
      }
    } break;
    case TS_LEAVE_OK: {
      if (response.HasShard2D() && response.HasAddrs()) {
        addrs_ = std::move(response.addrs);
        ports_ = std::move(response.ports);
        if (addrs_.size() != ports_.size()) {
          // FIXME Error
          return;
        }

        shard_2d_ = std::move(response.shard_2d);

        if (shard_2d_.empty()) return;

        for (size_t i = 0; i < addrs_.size(); ++i) {
          shard_clis_.emplace_back(shard_cli_loop_thr_.GetLoop(),
                                   InetAddr(addrs_[i], ports_[i]), this,
                                   shard_2d_[i]);
          shard_clis_[i].SetState(ShardClient::LEAVE_FROM);
          shard_clis_[i].Connect();
        }
      }
    } break;
    case TS_WAIT:
      cli_->GetLoop()->RunAfter([this]() { Join(); }, 0.5);
      break;
    }
  });

  cli_->SetConnectionCallback([this](TcpConnectionPtr const &conn) {
    if (conn->IsConnected()) {
      LOG_INFO << "Connect to the router successfully";
      LOG_INFO << "Then, join to the cluster that managed by the router";
      codec_.SetUpConnection(conn);
      conn_ = conn.get();
      Join();
    }
  });
}

void TrackerClient::Join()
{
  TrackRequest req;
  req.operation = protocol::TO_ADD_NODE;
  req.SetSharderPort();
  req.sharder_port = sharder_port_;
  codec_.Send(conn_, &req);
}

void TrackerClient::Leave()
{
  TrackRequest req;
  req.operation = protocol::TO_LEAVE;
  codec_.Send(conn_, &req);
}

void TrackerClient::MoveShardNode(uint32_t shard_id)
{
  TrackRequest req;
  req.operation = TO_MOVE_SHARD;
  req.SetShardId();
  req.shard_id = shard_id;
  req.SetNodeId();
  req.node_id = node_id_;
  codec_.Send(conn_, &req);
}

void TrackerClient::NotifyMoveFinish()
{
  TrackRequest req;
  req.operation = TO_MOVE_FINISH;
  codec_.Send(conn_, &req);
  for (auto &cli : shard_clis_) {
    cli.DisConnect();
  }
  shard_clis_.clear();
  LOG_INFO << "Move all shards finished";
}

void TrackerClient::SetAllShardNode()
{
  TrackRequest req;
  req.operation = TO_MOVE_SHARD;
  req.SetNodeId();
  req.node_id = node_id_;
  
  if (!shard_2d_.empty())
    req.SetShardId();

  for (auto &shards : shard_2d_) {
    assert(!shards.empty());
    for (auto shard : shards) {
      req.shard_id = shard;
      codec_.Send(conn_, &req);
    }
  }

  NotifyMoveFinish();
  StartSharder();
}

bool TrackerClient::IsTheFirstNode() const noexcept
{
  return addrs_.empty() && !shard_2d_.empty();
}

void TrackerClient::StartSharder()
{
  LOG_INFO << "Sharder start running";
  sharder_.Listen();
}
