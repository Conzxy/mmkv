#ifndef MMKV_SERVER_TRACKER_CLIENT_H_
#define MMKV_SERVER_TRACKER_CLIENT_H_

#include <kanon/net/user_client.h>

#include "mmkv/protocol/mmbp_codec.h"

namespace mmkv {
namespace server {

class TrackerClient : kanon::noncopyable {
 public:
  explicit TrackerClient(EventLoop *loop, InetAddr const &addr);

  TrackerClient(TrackerClient &&) = default;

  void Connect() { cli_->Connect(); }

  void GetMetaData();
  void SetShardNode(uint32_t shard_id);

  // For test
  void SetAllShardNode();
 private:
  void DebugPrint();

  TcpClientPtr cli_;
  protocol::MmbpCodec codec_;
  std::vector<std::vector<uint32_t>> shard_2d_;
  std::vector<std::string> addrs_;
  std::vector<uint16_t> ports_;
  uint32_t finish_node_num_ = 0;
  uint32_t node_id_;
  TcpConnection *conn_;
};

} // client
} // mmkv

#endif