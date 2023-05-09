// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#ifndef MMKV_SHARD_CONTROLLER_SERVER_H_
#define MMKV_SHARD_CONTROLLER_SERVER_H_

#include <unordered_map>
#include <queue>

#include <kanon/util/arithmetic_type.h>
#include <kanon/net/user_server.h>
#include <kanon/util/noncopyable.h>
#include <kanon/protobuf/protobuf_codec2.h>

#include "mmkv/algo/dictionary.h"
#include "mmkv/util/macro.h"
#include "mmkv/tracker/shard_controller_codec.h"

#include "type.h"

namespace mmkv {
namespace server {

class ShardControllerSession;
class Configd;

/**
 * Manage the all metadata about the cluster.
 */
class ShardControllerServer : kanon::noncopyable {
  friend class ShardControllerSession;
  friend class Configd;

 public:
  ShardControllerServer(EventLoop *loop, InetAddr const &addr);

  void Listen() { server_.StartRun(); }

  u64 GetShardNum() const noexcept { return shard_num_; }

  Configuration const *GetRecentConf() const noexcept
  {
    if (pending_conf_q_.empty()) return &config_;
    return &pending_conf_q_.front().conf;
  }

  Configuration *GetRecentConf() noexcept
  {
    if (pending_conf_q_.empty()) return &config_;
    return &pending_conf_q_.front().conf;
  }

 private:
  void UpdateConfig(Configuration &&conf);

  u64 GenerateNodeId() const;

  PendingConf &GetRecentPendingConf() noexcept { return pending_conf_q_.front(); }

  void PushPendingConf(PendingConf *conf) { pending_conf_q_.push(std::move(*conf)); }
  void PopPendingConf() { pending_conf_q_.pop(); }

  void CheckPendingConfSessionAndResponse();

 private:
  TcpServer            server_;
  ShardControllerCodec codec_;

  mutable kanon::MutexLock pending_conf_lock_;
  std::queue<PendingConf>  pending_conf_q_;
  Configuration            config_;

  std::unordered_map<node_id_t, ShardControllerSession *> node_session_map;

  struct PendingStateHash {
    MMKV_INLINE u64 operator()(PendingState state) const noexcept
    {
      return algo::Hash<int>()(state.state) ^ algo::Hash<node_id_t>()(state.node_id);
    }
  };

  struct PendingStateEqual {
    MMKV_INLINE bool operator()(PendingState lhs, PendingState rhs) const noexcept
    {
      // FIXME memcmp()?
      return lhs.state == rhs.state && lhs.node_id == rhs.node_id;
    }
  };

  algo::Dictionary<PendingState, std::weak_ptr<TcpConnection>, PendingStateHash, PendingStateEqual>
      pending_conf_conn_dict_;

  u64 shard_num_;

  Configd *p_configd_;
};

} // namespace server
} // namespace mmkv

#endif // MMKV_SERVER_TRACKER_H_
