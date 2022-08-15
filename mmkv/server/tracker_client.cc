#include "tracker_client.h"

#include "mmkv/protocol/track_response.h"
#include "mmkv/protocol/track_request.h"

#include <kanon/log/logger.h>

using namespace mmkv::server;
using namespace mmkv::protocol;
using namespace kanon;

TrackerClient::TrackerClient(EventLoop *loop, InetAddr const &addr)
  : cli_(NewTcpClient(loop, addr, "TrackerClient"))
  , codec_(TrackResponse::prototype)
{
  codec_.SetMessageCallback([this](TcpConnectionPtr const &conn, Buffer &buffer, uint32_t, TimeStamp recv_time) {
    TrackResponse response;
    response.ParseFrom(buffer);
    response.DebugPrint();

    switch (response.status_code) {
    case TS_ADD_NODE_OK: {
      if (response.HasAddrs() && response.HasShard2D() && response.HasNodeId()) {
        addrs_ = std::move(response.addrs);
        ports_ = std::move(response.ports);
        shard_2d_ = std::move(response.shard_2d);
        node_id_ = response.node_id;

        DebugPrint();
        SetAllShardNode();        
      }
    }
    break;
    case TS_WAIT:
      cli_->GetLoop()->RunAfter([this]() {
        GetMetaData();
      }, 0.5);
    break;
    }
  });

  cli_->SetConnectionCallback([this](TcpConnectionPtr const &conn) {
    if (conn->IsConnected()) {
      codec_.SetUpConnection(conn);
      // FIXME get() don't return const*?
      conn_ = conn.get();
      GetMetaData();
    }
  });
}

void TrackerClient::GetMetaData() {
  TrackRequest req;
  req.operation = protocol::TO_ADD_NODE;
  codec_.Send(conn_, &req);
}

void TrackerClient::SetShardNode(uint32_t shard_id) {
  TrackRequest req;
  req.operation = TO_MOVE_SHARD;
  req.SetShardId();
  req.shard_id = shard_id;
  req.SetNodeId();
  req.node_id = node_id_;
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

  TrackRequest req2;
  req2.operation = TO_MOVE_FINISH;
  codec_.Send(conn_, &req2);
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
    LOG_DEBUG << addrs_[i];
    for (auto shard : shard_2d_[i]) {
      LOG_DEBUG << "shard=" << shard;
    }
  }
#endif
}