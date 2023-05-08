// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#ifndef MMKV_SERVER_SHARD_CLIENT_H_
#define MMKV_SERVER_SHARD_CLIENT_H_

#include <kanon/net/user_client.h>

#include "kanon/protobuf/protobuf_codec2.h"

#include "mmkv/algo/hash_set.h"
#include "mmkv/algo/string.h"
#include "mmkv/tracker/common_type.h"
#include "mmkv/sharder/sharder_codec.h"

namespace mmkv {
namespace server {

class ShardControllerClient;
class Sharder;

class SharderClient : kanon::noncopyable {
  using Codec = SharderCodec;

 public:
  enum State : unsigned char {
    IDLE = 0,
    ADDING,  /** Adding to cluster */
    LEAVING, /** Leaving from cluster */
  };

  SharderClient(EventLoop *loop, InetAddr const &serv_addr, ShardControllerClient *controller_clie);

  void SetUp(Sharder *sharder, shard_id_t const *shard_ids, size_t shard_num, Codec *codec)
  {
    SetUpCodec(sharder, codec);
    SetUpShards(shard_ids, shard_num);
  }

  void SetUpShards(shard_id_t const *shard_ids, size_t shard_num) noexcept
  {
    assert(shard_num > 0);
    shard_ids_ = shard_ids;
    shard_num_ = shard_num;
  }

  void SetUpCodec(Sharder *sharder, Codec *codec);

  void Connect() { clie_->Connect(); }
  void DisConnect() { clie_->Disconnect(); }

  kanon::TcpConnectionPtr GetConnection() const { return clie_->GetConnection(); }

  /** Get shard from other sharder */
  void GetShard(Codec *codec, kanon::TcpConnection *conn, shard_id_t shard_id);

  /** Notify the shard remove all data from the sharder */
  void RemShard(Codec *codec, kanon::TcpConnection *conn, shard_id_t shard_id);

  void DelAllShards(Codec *codec, kanon::TcpConnection *conn);

  /** Put shard to other sharder */
  void PutShard(Sharder *sharder, Codec *codec, kanon::TcpConnection *conn, shard_id_t shard_id);

  void SetState(State state) noexcept { state_ = state; }

  State state() const noexcept { return state_; }

 private:
  TcpClientPtr   clie_;
  TcpConnection *conn_;

  ShardControllerClient *controller_clie_; /** Used for notifying */
  shard_id_t const      *shard_ids_;       /** The new or removed shards */
  size_t                 shard_num_;

  u64   shard_index_ = 0; /** Record the shard index that has handled */
  State state_       = IDLE;

  /* Push shard aux */
  std::vector<const algo::String *> send_keys_;
  size_t                            send_key_index_;
};

} // namespace server
} // namespace mmkv

#endif // MMKV_SERVER_SHARD_CLIENT_H_
