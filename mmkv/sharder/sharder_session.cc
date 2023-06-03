// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#include "internal/shard_session_impl.h"

SharderSession::SharderSession(TcpConnection *conn)
  : send_key_index_(0)
  , conn_(conn)
{
}

SharderSession::~SharderSession() noexcept {}

void SharderSession::SetUp(Sharder *sharder, Codec *codec)
{
  codec->SetMessageCallback(
      [sharder,
       codec,
       this](TcpConnectionPtr const &conn, Buffer &buffer, size_t payload_size, TimeStamp) {
        ShardRequest req;
        protobuf::ParseFromBuffer(&req, payload_size, &buffer);
        const shard_id_t shard_id = req.shard_id();
        switch (req.operation()) {
          case SHARD_OP_PULL:
            Impl::OnPullShard(sharder, this, codec, conn.get(), shard_id);
            break;
          case SHARD_OP_PUSH:
            Impl::OnPushShard(sharder, this, codec, conn.get(), req);
            break;
          case SHARD_OP_DEL:
            Impl::OnDelShard(sharder, this, codec, conn.get(), req);
            break;
        }
      }
  );
}

void SharderSession::PushShard(Sharder *sharder, shard_id_t shard_id)
{
  // TODO Really need conn_?
  Impl::OnPullShard(sharder, this, &sharder->codec_, conn_, shard_id);
}
