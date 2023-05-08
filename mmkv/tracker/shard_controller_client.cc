// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#include "shard_controller_client_impl.h"

ShardControllerClient::ShardControllerClient(
    EventLoop         *loop,
    InetAddr const    &addr,
    InetAddr const    &sharder_addr,
    std::string const &name,
    std::string const &sharder_name
)
  : cli_(NewTcpClient(loop, addr, name))
  , codec_(SHARDER_CONTROLLER_TAG, SHARDER_CONTROLLER_MAX_SIZE)
  , node_id_(INVALID_NODE_ID)
  , sharder_codec_(SHARDER_TAG, SHARDER_MAX_SIZE)
  , shard_cli_loop_thr_("SharderClients")
  , sharder_loop_thr_(sharder_name)
  , sharder_(sharder_loop_thr_.StartRun(), sharder_addr) // default port:9998
  , sharder_port_(sharder_addr.GetPort())
  , state_(IDLE)
{
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
            LOG_ERROR << "Incorrect state when receving completion wait";
        }
      } break; // CONTROL_STATUS_WAIT

      case CONTROL_STATUS_CONF_CHANGE: {
        switch (state()) {
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
      LOG_INFO << "Connect to the router successfully";
      LOG_INFO << "Then, join to the cluster that managed by the router";
      codec_.SetUpConnection(conn);
      conn_ = conn.get();
    }
  });
  shard_cli_loop_thr_.StartRun();
}

void ShardControllerClient::Join(Codec *codec, TcpConnection *const conn)
{
  ControllerRequest req;
  req.set_node_id(node_id_);
  req.set_operation(CONTROL_OP_ADD_NODE);
  req.set_sharder_port(sharder_port_);
  codec->Send(conn, &req);
}

void ShardControllerClient::Leave(Codec *codec, TcpConnection *const conn)
{
  ControllerRequest req;
  req.set_operation(CONTROL_OP_LEAVE_NODE);
  req.set_node_id(node_id_);
  codec->Send(conn, &req);
}

void ShardControllerClient::NotifyPullFinish()
{
  finish_node_num_++;
  if (finish_node_num_ == GetPeerNum()) {
    NotifyLeaveFinish();
  }
}

void ShardControllerClient::NotifyPushFinish()
{
  finish_node_num_++;

  if (finish_node_num_ == GetPeerNum()) {
    NotifyLeaveFinish();
  }
}

void ShardControllerClient::NotifyJoinFinish()
{
  ControllerRequest req = Impl::MakeRequest(this, CONTROL_OP_ADD_NODE_COMPLETE);
  Impl::Send(this, &req);
  finish_node_num_ = 0;
}

void ShardControllerClient::NotifyLeaveFinish()
{
  ControllerRequest req = Impl::MakeRequest(this, CONTROL_OP_LEAVE_NODE_COMPLETE);
  Impl::Send(this, &req);
  finish_node_num_ = 0;
}

void ShardControllerClient::StartSharder()
{
  sharder_.Listen();
  sharder_loop_thr_.StartRun();
  sharder_.controller = this;
}
