// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#include "../sharder_session.h"

#include "mmkv/sharder/sharder.h"

#include "mmkv/storage/db.h"
#include "mmkv/util/shard_util.h"
#include "mmkv/sharder/util.h"
#include "mmkv/sharder/sharder.h"
#include "mmkv/tracker/shard_controller_client.h"

using namespace mmkv::server;
using namespace mmkv::protocol;
using namespace std::placeholders;
using namespace mmkv::storage;
using namespace kanon;
using namespace mmkv::db;

struct SharderSession::Impl {
  MMKV_INLINE static void OnPullShard(
      Sharder             *sharder,
      SharderSession      *session,
      Codec               *codec,
      TcpConnection *const conn,
      shard_id_t           shard_id
  )
  {
    auto        *p_db_instance = &database_manager().GetShardDatabaseInstance(shard_id);
    auto        *p_db          = &p_db_instance->db;
    ShardMessage resp          = MakeShardResponse();

    {
      WLockGuard db_lock_guard(p_db_instance->lock);

      /* Don't care whether the shard does exists.
       * Database allow ignore the locked shard when executing command
       *
       * You must unlock the shard when the shard is deleted */
      p_db->LockShard(shard_id);
    }

    {
      /* The shard is locked, so the read lock is locked later is safe */
      RLockGuard db_lock_guard(p_db_instance->lock);
      auto       shard_code = p_db->GetShardKeys(shard_id, session->keys_);
      if (SC_OK == shard_code) {
        /* FIXME The shard is exists in database, but the shard can be incomplete.
         * We must check it first. */

        SerializeMmbpDataToSharderRequest(p_db, session->keys_, &session->send_key_index_, &resp);

        resp.set_shard_id(shard_id);
      } else if (SC_NO_SHARD == shard_code) {
        MutexGuard guard(sharder->pending_session_lock_);
        sharder->pending_shard_session_dict_.InsertKv(shard_id, session);
        resp.set_status(SHARD_STATUS_WAIT);
      }
    }

    if (session->keys_.size() == session->send_key_index_) {
      session->send_key_index_ = 0;
    }

    codec->Send(conn, &resp);
  }

  static MMKV_INLINE void OnPushShard(
      Sharder             *sharder,
      SharderSession      *session,
      Codec               *codec,
      TcpConnection *const conn,
      ShardRequest const  &req
  )
  {
    /* No need to lock the shard, because the shard must not be represented in the database */
    auto  shard_id      = req.shard_id();
    auto *p_db_instance = &database_manager().GetShardDatabaseInstance(shard_id);
    auto *p_db          = &p_db_instance->db;
    assert(!p_db->HasShard(shard_id));

    auto resp = MakeShardResponse();

    auto        data_num  = req.data_num();
    const char *p_data    = req.data().data();
    size_t      data_size = req.data().size();

    std::vector<MmbpRequest> mmbp_reqs;
    mmbp_reqs.resize(data_num);
    for (size_t i = 0; i < data_num; ++i) {
      auto p_old_data = p_data;
      mmbp_reqs[i].ParseFrom((void const **)&p_data, data_size);
      data_size -= p_data - p_old_data;
    }

    assert(data_size == 0);

    {
      WLockGuard guard(p_db_instance->lock);

      p_db->is_ignore_locked_shard = true;
      for (auto &mmbp_req : mmbp_reqs) {
        p_db_instance->Execute(mmbp_req, nullptr, 0 /* Dummy arg */);
      }
      p_db->is_ignore_locked_shard = false;
    }

    /* This session receive the PushShard Request indicates peer is a leaving node.
     * The session don't receive any PullShard request, hence, we only check the pending client.
     */
    MutexGuard guard(sharder->pending_client_lock_);
    auto       pending_client = sharder->pending_shard_client_dict_[shard_id];
    if (pending_client) {
      if (!sharder->canceling_client_set_.Find(pending_client)) {
        pending_client->PutShard(sharder, codec, shard_id);
      }
    }
  }

  MMKV_INLINE static void OnDelShard(
      Sharder             *sharder,
      SharderSession      *session,
      Codec               *codec,
      TcpConnection *const conn,
      ShardRequest const  &req
  )
  {
    auto const shard_id = req.shard_id();

    auto *p_db_instance = &database_manager().GetShardDatabaseInstance(shard_id);
    auto *p_db          = &p_db_instance->db;

    /* The shard must be pulled to another node
     * Don't check the state of this node is ok. */
    {
      WLockGuard guard(p_db_instance->lock);

      p_db->RemoveShard(shard_id);
      p_db->UnlockShard(shard_id);
    }
  }
};
