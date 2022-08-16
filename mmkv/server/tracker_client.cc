#include "tracker_client.h"

#include "mmkv/protocol/track_response.h"
#include "mmkv/protocol/track_request.h"

#include "config.h"
#include "shard_client.h"

#include <kanon/log/logger.h>

using namespace mmkv::server;
using namespace mmkv::protocol;
using namespace kanon;

TrackerClient::TrackerClient(EventLoop *loop, InetAddr const &addr, 
  uint16_t port, std::string const &name, std::string const &sharder_name)
  : cli_(NewTcpClient(loop, addr, name))
  , codec_(TrackResponse::prototype)
  , shard_cli_loop_thr_("ShardClients")
  , sharder_loop_thr_(sharder_name)
  , sharder_(sharder_loop_thr_.StartRun(), InetAddr(port)) // default port:9998
  , sharder_port_(port)
{
  shard_cli_loop_thr_.StartRun();
  codec_.SetMessageCallback([this](TcpConnectionPtr const &conn, Buffer &buffer, uint32_t, TimeStamp recv_time) {
    TrackResponse response;
    response.ParseFrom(buffer);
    response.DebugPrint();

    switch (response.status_code) {
    case TS_ADD_NODE_OK: {
      if (response.HasAddrs() && response.HasShard2D() && response.HasNodeId()) {
        addrs_ = std::move(response.addrs);
        ports_ = std::move(response.ports);
        if (addrs_.size() != ports_.size()) {
          // FIXME Error
          return;
        }

        shard_2d_ = std::move(response.shard_2d);
        node_id_ = response.node_id;

        DebugPrint();
        if (IsTheFirstNode()) {
          SetAllShardNode();
        } else {
          auto const sharder_num = addrs_.size();
          LOG_DEBUG << "Sharder num=" << sharder_num;
          shard_clis_.reserve(sharder_num);
          for (size_t i = 0; i < sharder_num; ++i) {
            shard_clis_.emplace_back(new ShardClient(shard_cli_loop_thr_.GetLoop(), InetAddr(addrs_[i], ports_[i]), this, shard_2d_[i]));
            shard_clis_[i]->Connect();
          }
        }
        
      }
    }
    break;
    case TS_WAIT:
      cli_->GetLoop()->RunAfter([this]() {
        Join();
      }, 0.5);
    break;
    }
  });

  cli_->SetConnectionCallback([this](TcpConnectionPtr const &conn) {
    if (conn->IsConnected()) {
      codec_.SetUpConnection(conn);
      // FIXME get() don't return const*?
      conn_ = conn.get();
      Join();
    }
  });
}

void TrackerClient::Join() {
  TrackRequest req;
  req.operation = protocol::TO_ADD_NODE;
  req.SetSharderPort();
  req.sharder_port = sharder_port_;
  codec_.Send(conn_, &req);
}

void TrackerClient::MoveShardNode(uint32_t shard_id) {
  TrackRequest req;
  req.operation = TO_MOVE_SHARD;
  req.SetShardId();
  req.shard_id = shard_id;
  req.SetNodeId();
  req.node_id = node_id_;
  codec_.Send(conn_, &req);
}

void TrackerClient::NotifyMoveFinish() {
  TrackRequest req;
  req.operation = TO_MOVE_FINISH;
  codec_.Send(conn_, &req);
}

void TrackerClient::SetAllShardNode() {
  TrackRequest req;
  req.operation = TO_MOVE_SHARD;
  req.SetNodeId();
  req.node_id = node_id_;
  for (auto &shards : shard_2d_) {
    for (auto shard : shards) {
      req.shard_id = shard;
      req.SetShardId();
      codec_.Send(conn_, &req);
    }
  }

  NotifyMoveFinish();
  StartSharder();
}

void TrackerClient::DebugPrint() {
#ifndef _NDEBUG
  if (addrs_.empty() && !shard_2d_.empty()) {
    LOG_DEBUG << "This is the first node";
    for (auto shard : shard_2d_[0]) {
      LOG_DEBUG << "shard=" << shard;
    }
    return;
  }

  LOG_DEBUG << "Address list: ";
  for (size_t i = 0; i < shard_2d_.size(); ++i) {
    LOG_DEBUG << addrs_[i] << ";" << ports_[i];
    for (auto shard : shard_2d_[i]) {
      LOG_DEBUG << "shard=" << shard;
    }
  }
#endif
}

bool TrackerClient::IsTheFirstNode() const noexcept {
  return addrs_.empty() && !shard_2d_.empty();
}

void TrackerClient::StartSharder() {
  LOG_INFO << "Sharder start running";
  sharder_.Listen();
}