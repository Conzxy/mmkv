// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#include "shard_controller_client.h"

#include "mmkv/tracker/type.h"
#include "mmkv/sharder/util.h"
#include "controller.pb.h"

#include "mmkv/storage/db.h"
#include "mmkv/server/config.h"

#include <kanon/log/logger.h>

using namespace mmkv::server;
using namespace kanon;
using namespace mmkv::storage;
using namespace ::kanon::protobuf;

struct ShardControllerClient::Impl {
  using Self = ShardControllerClient;

  MMKV_INLINE static ControllerRequest MakeRequest(Self *self, ControllerOperation op)
  {
    ControllerRequest req;
    req.set_node_id(self->node_id_);
    req.set_operation(op);
    return req;
  }

  MMKV_INLINE static void SendAllPeersRequest(
      Self                *ctler,
      size_t               peer_num,
      SharderClient::State state
  )
  {
    ctler->shard_clis_.reserve(peer_num);
    for (size_t i = 0; i < peer_num; ++i) {
      const auto &node_info = ctler->node_infos_[i];
      ctler->shard_clis_.emplace_back(
          ctler->cli_->GetLoop(),
          InetAddr(node_info.host(), node_info.port()),
          ctler
      );
      ctler->shard_clis_[i].SetUp(
          &ctler->sharder_,
          ctler->node_infos_[i].shard_ids().data(),
          ctler->node_infos_[i].shard_ids_size(),
          &ctler->sharder_codec_
      );
      ctler->shard_clis_[i].SetState(state);
      ctler->shard_clis_[i].Connect();
    }
  }

  MMKV_INLINE static void HandleJoinOk(
      ShardControllerClient    *ctl,
      ControllerResponse const &response
  )
  {
    const size_t peer_num = ctl->node_infos_.size();
    (void)peer_num;
    assert(response.has_shard_num());
    mmkv_config().shard_num = response.shard_num();

    // Controller return OK and no node infos indicates
    // this is the first node of the cluster.
    if (ctl->node_infos_.empty()) {
      if (!response.has_node_id()) {
        // FIXME Error return, untrusted peer
      }

      ctl->node_id_ = response.node_id();
      SetAllShardNode(ctl);
    } else {
#if 1
      auto const               &resp_node_infos = response.node_infos();
      algo::HashSet<shard_id_t> resp_hold_shard_id_set;
      for (auto const &resp_node_info : resp_node_infos) {
        ctl->shard_clis_.emplace_back(
            ctl->cli_->GetLoop(),
            InetAddr(resp_node_info.host(), resp_node_info.port()),
            ctl
        );

        auto const &resp_shard_ids = resp_node_info.shard_ids();
        for (auto const shard_id : resp_shard_ids) {
          resp_hold_shard_id_set.Insert(shard_id);
        }
        // If node has some shard don't present in the pull shards,
        // them must be pushed to other nodes.
        ctl->shard_clis_.back().SetState(SharderClient::PULLING);
        ctl->shard_clis_.back().Connect();
      }

      for (auto &db_ins : database_manager()) {
        WLockGuard guard(db_ins.lock);
        // We can know whether the keys has distributed or not.
        // For recovery, the keys has distributed.
        // For running forever, the keys can be distributed if config
        // contains the controller endpoint(ie. enable shard).
        //
        // But the key of dict and set is unique, so that's is ok.
        db_ins.db.DistributeKeysToShard();

        auto shard_begin = db_ins.db.ShardBegin();
        auto shard_end   = db_ins.db.ShardEnd();

        if (shard_begin != shard_end) {
          ControllerRequest req;
          for (; shard_end != shard_end; ++shard_begin) {
            if (!resp_hold_shard_id_set.Find(shard_begin->key)) {
              req.add_shard_ids(shard_begin->key);
            }
          }

          if (!req.shard_ids().empty()) {
            ctl->codec_.Send(ctl->conn_, &req);
            ctl->state_            = JOIN_PUSHING;
            // dummy value(must > 0 to prevent to call the NotifyJoinFinish())
            ctl->joining_push_num_ = 1;
          }
        }
        // Send the push shard ids to controller
      }
#else
      SendAllPeersRequest(ctler, peer_num, SharderClient::PULLING);
#endif
    }
  }

  MMKV_INLINE static void HandleJoinPushing(
      ShardControllerClient    *ctl,
      ControllerResponse const &resp
  )
  {
    const auto peer_num    = resp.node_infos_size();
    ctl->joining_push_num_ = peer_num;

    SendAllPeersRequest(ctl, peer_num, SharderClient::PUSHING);
  }

  MMKV_INLINE static void HandleLeaveOk(
      ShardControllerClient    *ctler,
      ControllerResponse const &resp
  )
  {
    const size_t peer_num = ctler->node_infos_.size();

    if (peer_num == 0) {
      LOG_DEBUG << "This is the last node of the cluster";
      ctler->NotifyLeaveFinish();
    } else {
      SendAllPeersRequest(ctler, peer_num, SharderClient::PUSHING);
    }
  }

  MMKV_INLINE static void HandleJoinConfChange(ShardControllerClient *ctler)
  {
    /* Remove the shards in peers */
    /* Notify the peers delete shards */
    auto pull_node_num = ctler->shard_clis_.size() - ctler->joining_push_num_;
    for (size_t i = 0; i < pull_node_num; ++i) {
      ctler->shard_clis_[i].DelAllShards(&ctler->sharder_codec_);
    }
    ctler->state_            = IDLE;
    ctler->joining_push_num_ = 0;
  }

  MMKV_INLINE static void HandleLeaveConfChange(ShardControllerClient *ctler)
  {
    /* Delete self all shards be sent */
    size_t cnt;
    for (auto &db_instance : database_manager()) {
      WLockGuard locked_shard_guard(db_instance.lock);

      db_instance.db.DeleteAll(&cnt);
      db_instance.db.UnlockAllShard();
    }

    ctler->state_ = IDLE;
  }

  /**
   * Only be called when the node is the first node of the cluster.
   * Because the first node no need to get shards from other sharder,
   * only notify the tracker.
   */
  MMKV_INLINE static void SetAllShardNode(ShardControllerClient *ctler)
  {
    const shard_id_t shard_num     = ctler->GetShardNum();
    const shard_id_t db_shard_num_ = shard_num / database_manager().size();
    shard_id_t       more_than_one = shard_num % database_manager().size();

    shard_id_t shard_idx = 0;
    for (auto &db : database_manager()) {
      WLockGuard guard(db.lock);
      auto       alloc_shard_num = db_shard_num_;
      if (more_than_one > 0) {
        alloc_shard_num++;
        more_than_one--;
      }

      while (alloc_shard_num--) {
        db.db.AddShard(shard_idx++);
      }

      db.db.DistributeKeysToShard();
    }

    assert(shard_idx == shard_num);
    ctler->NotifyJoinFinish();
    ctler->StartSharder();
  }

  MMKV_INLINE static void WaitConn(Self *ctler)
  {
    MutexGuard guard(ctler->conn_lock_);
    while (!ctler->conn_) {
      ctler->conn_cond_.Wait();
    }
  }
};
