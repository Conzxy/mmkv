// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#include "internal/shard_controller_client_impl.h"

#include "mmkv/server/option.h"

ShardControllerClient::ShardControllerClient(
    EventLoop         *loop,
    InetAddr const    &addr,
    InetAddr const    &sharder_addr,
    std::string const &name,
    std::string const &sharder_name
)
  : cli_(NewTcpClient(loop, addr, name))
  , conn_(nullptr)
  , codec_()
  , node_id_(INVALID_NODE_ID)
  , sharder_codec_()
  // , shard_cli_loop_thr_("SharderClients")
  , sharder_loop_thr_(sharder_name)
  , sharder_(sharder_loop_thr_.StartRun(), sharder_addr) // default port:9998
  , sharder_port_(sharder_addr.GetPort())
  , state_(IDLE)
  , conn_cond_(conn_lock_)
{
  cli_->EnableRetry();

  SharderClient::SetUpCodec(&sharder_, &sharder_codec_);

  codec_.SetMessageCallback([this](
                                TcpConnectionPtr const &conn,
                                Buffer                 &buffer,
                                size_t                  payload_size,
                                TimeStamp               recv_time
                            ) {
    ControllerResponse response;
    ParseFromBuffer(&response, payload_size, &buffer);

    switch (response.status()) {
      case CONTROL_STATUS_OK: {
        node_infos_ = std::move(*response.mutable_node_infos());

        switch (state()) {
          case JOINING: {
            Impl::HandleJoinOk(this, response);
          } break; // JOINGING

          case LEAVING: {
            Impl::HandleLeaveOk(this, response);
          } break; // LEAVING

          case JOIN_PUSHING: {
            Impl::HandleJoinPushing(this, response);
          } break;
          case IDLE:
          default:
            LOG_FATAL << "Recv controller status ok, state mustn't be IDLE";
            return;
        }
      } break;

      case CONTROL_STATUS_WAIT: {
        switch (state()) {
          case JOINING:
            LOG_DEBUG << "Join complete, waiting peer update configuration";

          case LEAVING:
            LOG_DEBUG << "Leave complete, waiting peer update configuration";
          case IDLE:
          default:
            LOG_DEBUG << "state = " << state();
            LOG_ERROR << "Incorrect state when receving completion wait";
        }
      } break; // CONTROL_STATUS_WAIT

      case CONTROL_STATUS_CONF_CHANGE: {
        switch (state()) {
          case JOIN_PUSHING:
          case JOINING: {
            Impl::HandleJoinConfChange(this);
          } break;

          case LEAVING: {
            Impl::HandleLeaveConfChange(this);
          } break;

          case IDLE: {

          } break;

          default:;
        }
      } break;

      case CONTROL_STATUS_HB: {
        LOG_DEBUG << "Recv a Heart Beat Packet";
      } break;

      case CONTROL_STATUS_NODE_FULL: {
        LOG_DEBUG << "The cluster is full, your can't join to it";
      } break;

      case CONTROL_STATUS_NODE_HAS_ADDED: {
        LOG_WARN << "The node has joined";
      } break;

      case CONTROL_STATUS_INVALID_MSG: {
        LOG_ERROR << "The controller request is invalid";
      } break;

      case CONTROL_STATUS_NODE_NON_EXISTS: {
        LOG_ERROR << "The node does not exists, you no need to leave the cluster";
      } break;
    }
  });

  cli_->SetConnectionCallback([this](TcpConnectionPtr const &conn) {
    if (conn->IsConnected()) {
      LOG_INFO << "Connect to the controller successfully";
      codec_.SetUpConnection(conn);
      {

        MutexGuard guard(conn_lock_);
        conn_ = conn.get();
        conn_cond_.Notify();
      }

      if (IsIdle()) {
        Join();
      }
    } else {
      MutexGuard guard(conn_lock_);
      conn_ = nullptr;
      LOG_INFO << "Disconnect to the controller";
    }
  });
  // shard_cli_loop_thr_.StartRun();

  // TODO When to start sharder?
  StartSharder();
}

void ShardControllerClient::Join()
{
  LOG_DEBUG << "Try send Join request...";
  Impl::WaitConn(this);
  ControllerRequest req;
  req.set_node_id(node_id_);
  req.set_operation(CONTROL_OP_ADD_NODE);
  req.set_sharder_port(sharder_port_);
  req.set_mmkvd_port(mmkv_option().port);

  LOG_INFO << "Controller request: " << req.DebugString();
  codec_.Send(cli_->GetConnection(), &req);
  state_ = JOINING;

  LOG_DEBUG << "Send Join request";
}

void ShardControllerClient::Leave()
{
  LOG_DEBUG << "Try send Leave request...";
  Impl::WaitConn(this);
  ControllerRequest req;
  req.set_operation(CONTROL_OP_LEAVE_NODE);
  req.set_node_id(node_id_);
  codec_.Send(conn_, &req);
  state_ = LEAVING;

  LOG_DEBUG << "Send Leave request";
}

void ShardControllerClient::NotifyPullFinish()
{
  finish_node_num_++;
  if (finish_node_num_ == GetPeerNum() + joining_push_num_) {
    NotifyJoinFinish();
  }
}

void ShardControllerClient::NotifyPushFinish()
{
  // JOIN_PUSHING will call this, redict it
  if (state_ == JOIN_PUSHING) {
    NotifyPullFinish();
    return;
  }

  finish_node_num_++;
  if (finish_node_num_ == GetPeerNum()) {
    NotifyLeaveFinish();
  }
}

void ShardControllerClient::NotifyJoinFinish()
{
  Impl::WaitConn(this);
  ControllerRequest req = Impl::MakeRequest(this, CONTROL_OP_ADD_NODE_COMPLETE);
  codec_.Send(conn_, &req);

#ifndef NDEBUG
  auto       p_db = &*database_manager().begin();
  WLockGuard guard(p_db->lock);
  size_t     shard_num = 0;
  for (auto beg = p_db->db.ShardBegin(); beg != p_db->db.ShardEnd(); ++beg) {
    // LOG_INFO << "Shard = " << beg->key;
    shard_num++;
  }
  LOG_INFO << "Shard num  = " << shard_num;
#endif
  finish_node_num_ = 0;
}

void ShardControllerClient::NotifyLeaveFinish()
{
  Impl::WaitConn(this);
  ControllerRequest req = Impl::MakeRequest(this, CONTROL_OP_LEAVE_NODE_COMPLETE);
  codec_.Send(conn_, &req);
  finish_node_num_ = 0;
}

void ShardControllerClient::StartSharder()
{
  sharder_.Listen();
  sharder_.controller = this;
}
