#ifndef MMKV_SERVER_TRACKER_H_
#define MMKV_SERVER_TRACKER_H_

#include <unordered_map>

#include <kanon/net/user_server.h>
#include <kanon/util/noncopyable.h>

namespace mmkv {
namespace server {

class TrackSession;

/**
 * Tracker is the shard controller of the entire cluster.
 * It manage the all metadata about the cluster.
 */
class Tracker : kanon::noncopyable {
  friend class TrackSession;

 public:
  using Node = uint32_t;
  using Shard = uint32_t;
  using Addrs = std::vector<std::string>;
  using Shards = std::vector<Shard>;

  explicit Tracker(EventLoop *loop, InetAddr const &addr);

  void Listen() { server_.StartRun(); }

  /*
   * Remove the node from the tracker.
   *
   * Different from the TrackSession::Leave(),
   * the method don't set response and don't wait
   * nodes get the dataset of the removed node
   *
   * Abnormal approach for removing a node from cluster
   */
  void RemoveNode(Node node);

  void RemovePendingNodes();

  /**
   * Used for router
   *
   * Router need send the target location to target node
   * \param key Key of the object of mmkv
   * \param[out] address target (ip) address
   * \param[out] port target port
   */
  void QueryKeyMappedAddress(std::string const &key, std::string &address,
                             uint16_t &port);

 private:
  struct ShardAndAddress {
    std::vector<Shard> shards;
    std::string address;
    uint16_t port;
  };

  enum State {
    IDLE = 0, /** idle state, can process join, etc. operation. */
    BUSY,     /** Contrary to the IDLE, can't process join, etc. */
  };

  TcpServer server_;
  std::vector<Node> shards_; /** shard --> node */
  std::unordered_map<Node, ShardAndAddress>
      node_map_; /** node --> shards and address */

  std::unordered_map<Node, TrackSession*> node_session_map;
  std::vector<Node> removed_nodes_;
  /* The node_id_ can not equal to node_num_ */

  int node_id_ = 0;  /** The id of unique node */
  int node_num_ = 0; /** The number of the nodes */

  State state_ = IDLE; /** The tracker state */
};

} // namespace server
} // namespace mmkv

#endif // MMKV_SERVER_TRACKER_H_