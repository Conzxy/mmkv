// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#ifndef MMKV_SERVER_TRACKER_CLIENT_H_
#define MMKV_SERVER_TRACKER_CLIENT_H_

#include <kanon/net/user_client.h>

#include "mmkv/server/config.h"

#include "controller.pb.h"
#include "mmkv/sharder/sharder_client.h"
#include "mmkv/sharder/sharder.h"
#include "mmkv/tracker/shard_controller_codec.h"

namespace mmkv {
namespace server {

/**
 * Interact to the tracker.
 * The main usage:
 * 1. Join the cluster
 * 2. Leave the cluster
 * 3. Move shard between new or leaved node
 */
class ShardControllerClient : kanon::noncopyable {
  using ControllerCodec = ShardControllerCodec;

 public:
  enum State {
    IDLE = 0,
    JOINING,
    LEAVING,
  };

  /**
   * \param addr Address of the tracker
   * \param shard_port Port of the sharder
   * \param name Name of the tracker client
   * \param sharder_name Name of the thread name of the sharder
   */
  explicit ShardControllerClient(
      EventLoop         *loop,
      InetAddr const    &addr,
      InetAddr const    &shard_addr,
      std::string const &name         = "ShardControllerClient",
      std::string const &sharder_name = "Sharder"
  );

  void Connect() { cli_->Connect(); }

  void DisConnect() { cli_->Disconnect(); }

  /*---------------------------------------*/
  /* Operation                             */
  /*---------------------------------------*/

  void Join();
  void Leave();

  /**
   * Start the sharder instance
   *
   * \note
   *  Must be called after the all new shards have been
   *  moved from other sharder
   */
  void StartSharder();

  shard_id_t GetShardNum() const noexcept { return mmkv_config().shard_num; }
  size_t     GetPeerNum() const noexcept { return shard_clis_.size(); }
  State      state() const noexcept { return state_; }

  bool IsIdle() const noexcept { return state_ == IDLE; }

  void NotifyPullFinish();
  void NotifyPushFinish();
  void NotifyJoinFinish();
  void NotifyLeaveFinish();

 private:
  friend struct Impl;
  struct Impl;

  TcpClientPtr    cli_;
  TcpConnection  *conn_;
  ControllerCodec codec_;

  /*------------------------------------*/
  /* Shards metadata                    */
  /*------------------------------------*/

  kanon::AtomicCounter<uint32_t> finish_node_num_{0};
  node_id_t                      node_id_; /** The id of the node */

  /*------------------------------------*/
  /* Sharder Clients                      */
  /*------------------------------------*/

  SharderCodec                                           sharder_codec_;
  ::google::protobuf::RepeatedPtrField<::mmkv::NodeInfo> node_infos_;

  // EventLoopThread            shard_cli_loop_thr_;
  // std::vector<std::unique_ptr<ShardClient>> shard_clis_;
  std::vector<SharderClient> shard_clis_;

  /*------------------------------------*/
  /* Sharder                            */
  /*------------------------------------*/

  EventLoopThread sharder_loop_thr_;
  Sharder         sharder_;
  uint16_t        sharder_port_;

  State state_;

  kanon::MutexLock conn_lock_;
  kanon::Condition conn_cond_;
};

} // namespace server
} // namespace mmkv

#endif
