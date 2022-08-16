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
  ShardClient(EventLoop *loop, InetAddr const &serv_addr, TrackerClient *tracker_client, std::vector<Shard> const &shards);
  
  void Connect() { cli_->Connect(); }
  void GetShard(Shard shard_id);
  void RemShard(Shard shard_id);

 private:
  TcpClientPtr cli_;
  protocol::MmbpCodec codec_;
  TcpConnection *conn_;
  TrackerClient *tracker_cli_;
  std::vector<Shard> const &shards_;
  uint32_t shard_front_ = 0;
};

} // server
} // mmkv

#endif // MMKV_SERVER_SHARD_CLIENT_H_