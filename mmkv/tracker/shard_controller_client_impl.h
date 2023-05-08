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

  MMKV_INLINE static void Send(Self *self, ::google::protobuf::Message const *msg)
  {
    self->codec_.Send(self->cli_->GetConnection(), msg);
  }

  MMKV_INLINE static void HandleJoinOk(
      ShardControllerClient    *clt,
      ControllerResponse const &response
  )
  {
    const size_t peer_num = clt->node_infos_.size();
    assert(response.has_shard_num());
    mmkv_config().shard_num = response.shard_num();

    // Controller return OK and no node infos indicates
    // this is the first node of the cluster.
    if (clt->node_infos_.empty()) {
      if (!response.has_node_id()) {
        // FIXME Error return, untrusted peer
      }

      clt->node_id_ = response.node_id();
      SetAllShardNode(clt);
    } else {
      clt->shard_clis_.reserve(peer_num);
      for (size_t i = 0; i < peer_num; ++i) {
        const auto &node_info = clt->node_infos_[i];
        clt->shard_clis_.emplace_back(
            clt->shard_cli_loop_thr_.GetLoop(),
            InetAddr(node_info.host(), node_info.port()),
            clt
        );
        clt->shard_clis_[i].SetUp(
            &clt->sharder_,
            clt->node_infos_[i].shard_ids().data(),
            clt->node_infos_[i].shard_ids_size(),
            &clt->sharder_codec_
        );
        clt->shard_clis_[i].SetState(SharderClient::ADDING);
        clt->shard_clis_[i].Connect();
      }
    }
  }

  MMKV_INLINE static void HandleLeaveOk(ShardControllerClient *clt, ControllerResponse const &resp)
  {
  }

  MMKV_INLINE static void HandleJoinConfChange(ShardControllerClient *clt)
  {
    /* Remove the shards in peers */
    /* Notify the peers delete shards */
    for (auto &sharder_cli : clt->shard_clis_) {
      sharder_cli.DelAllShards(&clt->sharder_codec_, sharder_cli.GetConnection().get());
    }
    clt->state_ = IDLE;
  }

  MMKV_INLINE static void HandleLeaveConfChange(ShardControllerClient *clt)
  {
    /* Delete self all shards be sent */
    size_t cnt;
    for (auto &db_instance : database_manager()) {
      WLockGuard locked_shard_guard(db_instance.lock);

      db_instance.db.DeleteAllShard();
      db_instance.db.DeleteAll(&cnt);
      db_instance.db.UnlockAllShard();
    }
    clt->state_ = IDLE;
  }

  /**
   * Only be called when the node is the first node of the cluster.
   * Because the first node no need to get shards from other sharder,
   * only notify the tracker.
   */
  MMKV_INLINE static void SetAllShardNode(ShardControllerClient *clt)
  {
    const shard_id_t shard_num     = clt->GetShardNum();
    const shard_id_t db_shard_num_ = shard_num / database_manager().size();
    shard_id_t       more_than_one = shard_num % database_manager().size();

    shard_id_t shard_idx = 0;
    for (auto &db : database_manager()) {
      auto alloc_shard_num = db_shard_num_;
      if (more_than_one--) {
        alloc_shard_num++;
      }

      while (alloc_shard_num--) {
        db.db.AddShard(shard_idx++);
      }
    }

    assert(shard_idx == shard_num);
    clt->NotifyJoinFinish();
    clt->StartSharder();
  }
};
