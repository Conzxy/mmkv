#include "shard_client.h"

#include "mmkv/protocol/shard_args.h"
#include "mmkv/protocol/shard_reply.h"
#include "mmkv/tracker/tracker_client.h"
#include "mmkv/util/shard_util.h"

using namespace mmkv::server;
using namespace mmkv::protocol;

ShardClient::ShardClient(EventLoop *loop, InetAddr const &serv_addr,
                         TrackerClient *tracker_client,
                         std::vector<Shard> const &shards)
  : cli_(NewTcpClient(loop, serv_addr, "ShardClient"))
  , conn_(nullptr)
  , codec_(ShardReply::prototype)
  , tracker_cli_(tracker_client)
  , shards_(&shards)
{
  codec_.SetMessageCallback([this](TcpConnectionPtr const &conn, Buffer &buffer,
                                   uint32_t, TimeStamp) {
    ShardReply reply;
    reply.ParseFrom(buffer);
    reply.DebugPrint();

    switch (reply.GetShardCode()) {
    case SC_OK:
#if SHARD_TEST
      LOG_INFO << "GetShard success";
      // Now, the shard is duplicated in this node
      // we can notify the tracker modify the node id corresponding shard
      tracker_cli_->MoveShardNode((*shards_)[shard_front_]);
      // Notify the sharder remove the shard from it
      RemShard((*shards_)[shard_front_]);
      shard_front_++;
      if (shard_front_ == shards_->size()) {
        LOG_INFO << "GetShard complete";
        tracker_cli_->NotifyMoveFinish();
        if (state_ == JOIN_TO) {
          tracker_cli_->StartSharder();
        }
        shard_front_ = 0;
      }
#else

#endif
      break;

    case SC_PUT_SHARD_OK: {
#if SHARD_TEST
      LOG_INFO << "Put shard successfully";
      // Now, other sharder holds same shard
      // Remove self shard
      tracker_cli_->MoveShardNode((*shards_)[shard_front_]);
      shard_front_++;
      if (shard_front_ == shards_->size()) {
        LOG_INFO << "PutShard complete";
        tracker_cli_->NotifyMoveFinish();
        shard_front_ = 0;
      }
#endif
    }

    case SC_NO_SHARD:
      LOG_ERROR << "No shard in the shard server "
                << cli_->GetServerAddr().ToIpPort();
      break;

    case SC_NOT_SHARD_SERVER:
      LOG_ERROR << cli_->GetServerAddr().ToIpPort() << " is not a shard server";
      break;
    default:
      LOG_ERROR << "Unknown ShardCode";
    }
  });

  cli_->SetConnectionCallback([this](TcpConnectionPtr const &conn) {
    if (conn->IsConnected()) {
      conn_ = conn.get();
      codec_.SetUpConnection(conn);
      LOG_DEBUG << "Shard num = " << shards_->size();
      for (size_t i = 0; i < shards_->size(); ++i) {
        if (state_ == JOIN_TO)
          GetShard((*shards_)[i]);
        else if (state_ == LEAVE_FROM)
          PutShard((*shards_)[i]);
      }
    }
  });
}

void ShardClient::GetShard(Shard shard_id)
{
  assert(conn_);
  ShardArgs args;
  args.operation = SO_GET_SHARD;
  args.shard_id = shard_id;
  codec_.Send(conn_, &args);
}

void ShardClient::RemShard(Shard shard_id)
{
  assert(conn_);
  ShardArgs args;
  args.operation = SO_REM_SHARD;
  args.shard_id = shard_id;
  codec_.Send(conn_, &args);
}

void ShardClient::PutShard(Shard shard_id)
{
  assert(conn_);
  ShardArgs args;
  args.operation = SO_PUT_SHARD;
  args.shard_id = shard_id;
  codec_.Send(conn_, &args);
}