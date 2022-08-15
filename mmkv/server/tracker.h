#ifndef MMKV_SERVER_TRACKER_H_
#define MMKV_SERVER_TRACKER_H_

#include <unordered_map>

#include <kanon/util/noncopyable.h>
#include <kanon/net/user_server.h>

#include "config.h"

namespace mmkv {
namespace server {

class Tracker : kanon::noncopyable {
  friend class TrackSession;
 public:
  using Node = uint32_t;
  using Shard = uint32_t;
  using Addrs = std::vector<std::string>;
  using Shards = std::vector<Shard>;

  explicit Tracker(EventLoop *loop, 
    InetAddr const &addr = InetAddr(g_config.tracker_port)); 

  void Listen() {
    server_.StartRun();
  }
 private:

  struct ShardAndAddress {
    std::vector<Shard> shards;
    std::string address;
    uint16_t port;
  };

  enum State {
    IDLE = 0,
    BUSY,
  };

  TcpServer server_;
  std::vector<Node> shards_; // shard --> node
  std::unordered_map<Node, ShardAndAddress> node_map; // 
  int node_id_ = 0;

  State state_ = IDLE;
};

} // server
} // mmkv

#endif // MMKV_SERVER_TRACKER_H_