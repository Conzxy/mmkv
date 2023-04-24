// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#ifndef MMKV_SERVER_SHARD_CLIENT_H_
#define MMKV_SERVER_SHARD_CLIENT_H_

#include <kanon/net/user_client.h>

#include "mmkv/protocol/mmbp_codec.h"

namespace mmkv {
namespace server {

using protocol::Shard;

class TrackerClient;

class ShardClient : kanon::noncopyable {
 public:
  enum State : unsigned char {
    IDLE = 0,
    JOIN_TO, /** Joining to cluster */
    LEAVE_FROM, /** Leaving from cluster */
  };

  ShardClient(EventLoop *loop, InetAddr const &serv_addr,
              TrackerClient *tracker_client, std::vector<Shard> const &shards);

  void Connect()
  {
    cli_->Connect();
  }
  void DisConnect()
  {
    cli_->Disconnect();
  }

  /** Get shard from other sharder */
  void GetShard(Shard shard_id);

  /** Notify the shard remove all data from the sharder */
  void RemShard(Shard shard_id);
  
  /** Put shard to other sharder */
  void PutShard(Shard shard_id);

  void SetState(State state) noexcept
  {
    state_ = state;
  }

 private:
  TcpClientPtr cli_;
  TcpConnection *conn_;
  protocol::MmbpCodec codec_;

  TrackerClient *tracker_cli_; /** Used for notifying */
  std::vector<Shard> const *shards_; /** The new or removed shards */

  uint32_t shard_front_ = 0; /** Record the shard index that has handled */
  State state_ = IDLE;
};

} // namespace server
} // namespace mmkv

#endif // MMKV_SERVER_SHARD_CLIENT_H_
