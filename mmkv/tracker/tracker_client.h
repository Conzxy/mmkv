// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#ifndef MMKV_SERVER_TRACKER_CLIENT_H_
#define MMKV_SERVER_TRACKER_CLIENT_H_

#include <kanon/net/user_client.h>

#include "mmkv/protocol/mmbp_codec.h"
#include "mmkv/server/config.h"

#include "mmkv/sharder/shard_client.h"
#include "mmkv/sharder/sharder.h"

namespace mmkv {
namespace server {

/**
 * Interact to the tracker.
 * The main usage:
 * 1. Join the cluster
 * 2. Leave the cluster
 * 3. Move shard between new or leaved node
 */
class TrackerClient : kanon::noncopyable {
 public:
  /**
   * \param addr Address of the tracker
   * \param shard_port Port of the sharder
   * \param name Name of the tracker client
   * \param sharder_name Name of the thread name of the sharder
   */
  explicit TrackerClient(EventLoop *loop, InetAddr const &addr, 
    InetAddr const &shard_addr,
    std::string const &name = "TrackerClient",
    std::string const &sharder_name = "Sharder");

  void Connect() { cli_->Connect(); }
  void DisConnect() { cli_->Disconnect(); }
  
  /*---------------------------------------*/
  /* Operation                             */
  /*---------------------------------------*/

  void Join();
  void Leave();
  void MoveShardNode(uint32_t shard_id);
  
  /** Notify the tracker the operation is complete */
  void NotifyMoveFinish();

  /** 
   * Start the sharder instance 
   * 
   * \note
   *  Must be called after the all new shards have been
   *  moved from other sharder
   */
  void StartSharder();
  
  /**
   * Only be called when the node is the first node of the cluster.
   * Because the first node no need to get shards from other sharder,
   * only notify the tracker.
   */
  void SetAllShardNode();
 private:
  /** Determine the node is the first node of the cluster */
  bool IsTheFirstNode() const noexcept;

  TcpClientPtr cli_;
  TcpConnection *conn_;
  protocol::MmbpCodec codec_;
  
  /*------------------------------------*/
  /* Shards metadata                    */
  /*------------------------------------*/

  std::vector<std::vector<uint32_t>> shard_2d_;
  std::vector<std::string> addrs_;
  std::vector<uint16_t> ports_;

  uint32_t finish_node_num_ = 0;
  uint32_t node_id_; /** The id of the node(for identify and debugging) */
  
  /*------------------------------------*/
  /* Sharder Clients                      */
  /*------------------------------------*/

  EventLoopThread shard_cli_loop_thr_;
  // std::vector<std::unique_ptr<ShardClient>> shard_clis_;
  std::vector<ShardClient> shard_clis_;
  
  /*------------------------------------*/
  /* Sharder                            */
  /*------------------------------------*/

  EventLoopThread sharder_loop_thr_;
  Sharder sharder_;
  uint16_t sharder_port_;
};

} // client
} // mmkv

#endif
