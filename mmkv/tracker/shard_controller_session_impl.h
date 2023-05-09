// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#include "shard_controller_session.h"

#include <kanon/protobuf/protobuf_codec2.h>

#include "mmkv/util/macro.h"
#include "controller.pb.h"
#include "shard_controller_server.h"

using namespace mmkv::server;
using namespace kanon;
using namespace kanon::protobuf;

struct ShardControllerSession::Impl {
  MMKV_INLINE static ControllerResponse MakeControllerResponse(ControllerStatusCode status)
  {
    ControllerResponse resp;
    resp.set_status(status);
    return resp;
  }

  MMKV_INLINE static void SendRejectResponse(
      ProtobufCodec2      *codec,
      TcpConnection *const conn,
      ControllerStatusCode status
  )
  {
    auto req = Impl::MakeControllerResponse(status);
    codec->Send(conn, &req);
  }

  MMKV_INLINE static void ControllorOperationComplete(
      ShardControllerSession *p_session,
      ShardControllerServer  *server,
      TcpConnectionPtr const &conn,
      ShardControllerCodec   *codec,
      ControllerRequest      &req,
      ConfState               conf_state
  )
  {
    const auto         node_id = req.node_id();
    ControllerResponse resp;

    {
      MutexGuard guard(server->pending_conf_lock_);
      auto       p_recent_conf = server->GetRecentPendingConf();
      if (p_recent_conf.node_id == node_id && p_recent_conf.state == conf_state) {
        resp.set_status(CONTROL_STATUS_CONF_CHANGE);
        server->UpdateConfig(std::move(p_recent_conf.conf));
        server->PopPendingConf();

        // Check pending
        server->CheckPendingConfSessionAndResponse();
      } else {
        resp.set_status(CONTROL_STATUS_WAIT);

        server->pending_conf_conn_dict_.InsertKv(PendingState{conf_state, node_id}, conn);
      }
    }

    codec->Send(conn, &resp);
  }
};
