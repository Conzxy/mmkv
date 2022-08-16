#ifndef MMKV_SERVER_TRACKER_CLIENT_H_
#define MMKV_SERVER_TRACKER_CLIENT_H_

#include <kanon/net/user_client.h>

#include "mmkv/protocol/mmbp_codec.h"
#include "config.h"

#include "shard_client.h"
#include "sharder.h"

namespace mmkv {
namespace server {

class TrackerClient : kanon::noncopyable {
 public:
  /**
   * \param sharder_port For test
   * \param name For test
   */
  explicit TrackerClient(EventLoop *loop, InetAddr const &addr, 
    uint16_t sharder_port = g_config.sharder_port,
    std::string const &name = "TrackerClient",
    std::string const &sharder_name = "Sharder");

  void Connect() { cli_->Connect(); }

  void Join();
  void MoveShardNode(uint32_t shard_id);

  void NotifyMoveFinish();
  void StartSharder();

  // For test
  void SetAllShardNode();
 private:
  bool IsTheFirstNode() const noexcept;

  void DebugPrint();

  TcpClientPtr cli_;
  protocol::MmbpCodec codec_;
  std::vector<std::vector<uint32_t>> shard_2d_;
  std::vector<std::string> addrs_;
  std::vector<uint16_t> ports_;
  uint32_t finish_node_num_ = 0;
  uint32_t node_id_;
  TcpConnection *conn_;

  EventLoopThread shard_cli_loop_thr_;
  std::vector<std::unique_ptr<ShardClient>> shard_clis_;
  EventLoopThread sharder_loop_thr_;
  Sharder sharder_;
  uint16_t sharder_port_;
};

} // client
} // mmkv

#endif