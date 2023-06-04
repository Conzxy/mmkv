// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#include "sharder_client.h"

#include "mmkv/sharder/util.h"
#include "mmkv/tracker/shard_controller_client.h"
#include "mmkv/storage/db.h"
#include "mmkv/protocol/shard_code.h"
#include "mmkv/sharder/sharder_session.h"
#include "mmkv/util/shard_util.h"

using namespace kanon;
using namespace mmkv::server;
using namespace ::kanon::protobuf;
using namespace mmkv::protocol;
using namespace mmkv::storage;

SharderClient::SharderClient(
    EventLoop             *loop,
    InetAddr const        &serv_addr,
    ShardControllerClient *controller_clie
)
  : clie_(NewTcpClient(loop, serv_addr, "SharderClient"))
  , controller_clie_(controller_clie)
  , shard_ids_(nullptr)
  , shard_num_(0)
  , shard_index_(0)
  , send_keys_(0)
{
}

void SharderClient::SetUpCodec(Sharder *sharder, Codec *codec)
{
  codec->SetMessageCallback(
      [sharder](TcpConnectionPtr const &conn, Buffer &buffer, size_t payload_size, TimeStamp) {
        auto resp = MakeShardResponse();
        ParseFromBuffer(&resp, payload_size, &buffer);

        auto sharder_cli = AnyCast<SharderClient>(conn->GetContext());

        switch (resp.status()) {
          case SHARD_STATUS_OK: {
            switch (sharder_cli->state()) {
              case PULLING: {
                // assert(shard_index_ == resp.shard_id());
                LOG_DEBUG << "Pull shard [" << sharder_cli->shard_index_ << "] successfully";

                // assert(resp.has_data() && resp.has_data_num());
                const shard_id_t shard_id = resp.shard_id();
                auto            *p_db     = &database_manager().GetShardDatabaseInstance(shard_id);

                {
                  WLockGuard guard(p_db->lock);
                  p_db->db.AddShard(shard_id);
                }

                if (resp.has_data() && resp.has_data_num()) {
                  const size_t data_num = resp.data_num();
                  void const  *p_data   = &resp.data()[0];
                  size_t       index    = resp.data().size();
                  for (size_t i = 0; i < data_num; ++i) {
                    /* The request has some members are unsafe after be moved. */
                    MmbpRequest request;

                    /* The ParseFrom() is override in the virtual table,
                     * the non-virtual ParseFrom() isn't inherited implicitly.
                     *
                     * You can use "useing Base::ParseFrom()" to import this
                     * but I don't likt it and rename it to ParseFrom2()
                     */
                    request.ParseFrom2(&p_data, &index);
                    p_db->Execute(request, nullptr, 0);
                  }
                }

                sharder_cli->shard_index_++;
                if (sharder_cli->shard_index_ == sharder_cli->shard_num_) {
                  LOG_DEBUG << "Pull shards complete";
                  sharder_cli->controller_clie_->NotifyPullFinish();
                  sharder_cli->shard_index_ = 0;
                }

                /* Currently, node in ADDING state.
                 * This indicates the node is a new node.
                 * We only check the pending session is enough.
                 *
                 * since the PushShard() actively only when the node is leaving.
                 */
                MutexGuard guard(sharder->pending_session_lock_);
                auto      *p_pending_session = sharder->pending_shard_session_dict_[shard_id];

                if (p_pending_session) {
                  if (!sharder->canceling_sessions_set_.Find(p_pending_session)) {
                    p_pending_session->PushShard(sharder, shard_id);
                  }
                }

              } break;

              case PUSHING: {
                LOG_DEBUG << "Push shard [" << sharder_cli->shard_index_ << "] successfully";
                assert(sharder_cli->shard_index_ == resp.shard_id());
                sharder_cli->shard_index_++;
                if (sharder_cli->shard_index_ == sharder_cli->shard_num_) {
                  LOG_DEBUG << "Push shards complete";
                  sharder_cli->controller_clie_->NotifyPushFinish();
                }
              } break; // state()

              case IDLE:
              default:
                LOG_FATAL << "Recv sharder status, state mustn't be IDLE";
                return;
            }
          } break; // SHARD_STATUS_OK

          case SHARD_STATUS_WAIT: {
          } break;
        }
      }
  );
}

void SharderClient::SetUp(
    Sharder          *sharder,
    shard_id_t const *shard_ids,
    size_t            shard_num,
    Codec            *codec
)
{
  clie_->SetConnectionCallback([sharder, codec, this](TcpConnectionPtr const &conn) {
    if (conn->IsConnected()) {
      conn_ = conn.get();
      codec->SetUpConnection(conn);
      LOG_DEBUG << "Shard num = " << shard_num_;
      for (size_t i = 0; i < shard_num_; ++i) {
        if (state_ == PULLING)
          GetShard(codec, conn.get(), shard_ids_[i]);
        else if (state_ == PUSHING)
          PutShard(sharder, codec, conn.get(), shard_ids_[i]);
      }
      conn->SetContext(this);
    } else {
      LOG_DEBUG << "The Sharder Client: [" << conn->GetName() << "] is down";
      sharder->canceling_client_set_.Erase(this);
      conn->SetContext(nullptr);
    }
  });
  SetUpShards(shard_ids, shard_num);
}

void SharderClient::GetShard(Codec *codec, kanon::TcpConnection *conn, shard_id_t shard_id)
{
  auto req = MakeShardRequest();
  req.set_operation(SHARD_OP_PULL);
  req.set_shard_id(shard_id);
  codec->Send(conn, &req);
}

void SharderClient::RemShard(Codec *codec, kanon::TcpConnection *conn, shard_id_t shard_id)
{
  auto req = MakeShardRequest();
  req.set_operation(SHARD_OP_DEL);
  req.set_shard_id(shard_id);
  codec->Send(conn, &req);
}

void SharderClient::DelAllShards(Codec *codec)
{
  assert(
      controller_clie_->state() == ShardControllerClient::JOINING ||
      controller_clie_->state() == ShardControllerClient::JOIN_PUSHING
  );

  /* FIXME Batcing send */
  for (size_t i = 0; i < shard_num_; ++i) {
    RemShard(codec, conn_, shard_ids_[i]);
  }
}

void SharderClient::PutShard(
    Sharder              *sharder,
    Codec                *codec,
    kanon::TcpConnection *conn,
    shard_id_t            shard_id
)
{
  auto      req           = MakeShardRequest();
  auto     *p_db_instance = &storage::database_manager().GetShardDatabaseInstance(shard_id);
  auto     *p_db          = &p_db_instance->db;
  ShardCode code;

  {
    WLockGuard guard(p_db_instance->lock);
    /* FIXME unlock when all shards are pushed */
    p_db->LockShard(shard_id);
  }

  p_db_instance->lock.RLock();
  code = p_db->GetShardKeys(shard_id, send_keys_);
  /* TODO SharderSession(PushRequest) and SharderClient(PullRequest) must call this to send pending
   * shard */
  if (SC_NO_SHARD == code) {
    p_db_instance->lock.RUnlock();

    MutexGuard guard(sharder->pending_client_lock_);
    sharder->pending_shard_client_dict_.InsertKv(shard_id, this);
  } else if (SC_OK == code) {
    // Send shards to peer
    SerializeMmbpDataToSharderRequest(p_db, send_keys_, &send_key_index_, &req);

    p_db_instance->lock.RUnlock();
  }

  req.set_operation(SHARD_OP_PUSH);
  req.set_shard_id(shard_id);
  codec->Send(conn, &req);
}

void SharderClient::PutShard(Sharder *sharder, Codec *codec, shard_id_t shard_id)
{
  PutShard(sharder, codec, conn_, shard_id);
}
