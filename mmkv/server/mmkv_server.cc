// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#include "mmkv_server.h"

#include "mmkv/disk/recover.h"
#include "mmkv/util/time_util.h"

#include "mmkv/storage/db.h"
#include "mmkv/server/config.h"
#include "mmkv/algo/hash_util.h"
#include "mmkv/protocol/mmbp.h"
#include "mmkv/protocol/mmbp_request.h"
#include "mmkv/protocol/mmbp_response.h"
#include "mmkv/protocol/mmbp_type.h"
#include "mmkv/util/macro.h"
#include "mmkv/util/conv.h"
#include "mmkv/storage/db.h"

#include "mmkv/disk/request_log.h"
#include "mmkv/disk/log_command.h"
#include "common.h"

#include <kanon/util/ptr.h>

using namespace kanon;
using namespace mmkv::server;
using namespace mmkv::disk;
using namespace mmkv::server;
using namespace mmkv::storage;
using namespace mmkv::util;
using namespace mmkv::protocol;
using namespace mmkv;

static void LogRequestToFile(Buffer &buffer, uint32_t request_len);

MmkvServer::MmkvServer(EventLoop *loop, InetAddr const &addr, InetAddr const &sharder_addr)
  : server_(loop, addr, "Mmkv")
  , codec_(protocol::MmbpRequest::GetPrototype())
  // , tracker_cli_loop_thr_(mmkv_config().IsSharder()
  //                             ? new EventLoopThread("TrackerClient")
  //                             : nullptr)
  , ctler_cli_(
        mmkv_config().IsSharder() ? new ShardControllerClient(
                                        loop,
                                        InetAddr(mmkv_config().shard_controller_endpoint),
                                        InetAddr(mmkv_config().sharder_endpoint)
                                    )
                                  : nullptr
    )
{
  server_.SetConnectionCallback([this](TcpConnectionPtr const &conn) {
    if (conn->IsConnected()) {
      LOG_MMKV(conn) << " connected";
      codec_.SetUpConnection(conn);
    } else {
      // auto p = AnyCast<MmkvSession>(conn->GetContext());
      // assert(p);

      // delete p;
      LOG_MMKV(conn) << " disconnected";
    }
  });

  codec_.SetMessageCallback([this](
                                TcpConnectionPtr const &conn,
                                Buffer                 &buffer,
                                uint32_t                request_len,
                                TimeStamp               recv_time
                            ) {
    // Set g_recv_time for expireafter and expiremafter
    database_manager().SetRecvTime(recv_time.GetMicrosecondsSinceEpoch() / 1000);

    MmbpRequest request;

    // TODO Modify the recover logic of SHRAD_LEAVE/JOIN
    if (mmkv_config().log_method == LM_REQUEST) {
      LogRequestToFile(buffer, request_len);
    }

    request.ParseFrom(buffer);
    request.DebugPrint();

    LOG_MMKV(conn) << " " << GetCommandString((Command)request.command);

    if (request.HasKey()) {
      LOG_MMKV(conn) << " "
                     << "key: " << request.key;
    }

    MmbpResponse response;
    if (request.command == SHARD_JOIN) {
      if (!request.HasKey() || !request.HasCount()) {
        MmbpResponse resp;
        resp.status_code = StatusCode::S_INVALID_REQUEST;
      } else {
        InetAddr controller_addr(request.key.c_str(), (uint16_t)request.count);
        ctler_cli_.reset(new ShardControllerClient(
            server_.GetLoop(),
            InetAddr(mmkv_config().shard_controller_endpoint),
            InetAddr(mmkv_config().sharder_endpoint)
        ));
      }
    } else if (request.command == SHARD_LEAVE) {
      if (ctler_cli_) {
        ctler_cli_->Leave();
      } else {
        // TODO
      }
    } else {
      database_manager().Execute(request, &response);
    }

    response.DebugPrint();
    codec_.Send(conn, &response);

    LOG_MMKV(conn) << " " << response.status_code << " "
                   << StatusCode2Str((StatusCode)response.status_code);
  });

  if (ctler_cli_) {
    LOG_INFO << "This is configured as a shard server also";
    LOG_INFO << "Connecting to the router server...";
    ctler_cli_->Connect();
  }
}

MmkvServer::~MmkvServer() noexcept {}

void MmkvServer::Start()
{
  if (mmkv_config().log_method == LM_REQUEST) {
    auto stime = GetTimeMs();
    LOG_INFO << "Recover from request log";
    try {
      Recover recover;
      recover.ParseFromRequest();
      LOG_INFO << "Recover complete";
    }
    catch (FileException const &ex) {
      LOG_ERROR << ex.what();
      LOG_ERROR << "Can't recover database from log";
    }
    LOG_INFO << "Recover cost: " << (GetTimeMs() - stime) << "ms";
    rlog().Start();
  }

  if (mmkv_config().expiration_check_cycle > 0) {
    LOG_INFO << "The mmkv will check all expired entries actively";
    LOG_INFO << "The cycle is " << mmkv_config().expiration_check_cycle << " seconds";

    server_.GetLoop()->RunEvery(
        []() {
          // FIXME thread-safe
          LOG_DEBUG << "Check expiration";
          database_manager().CheckExpirationCycle();
        },
        mmkv_config().expiration_check_cycle
    );
  }

  Listen();
}

static inline void LogRequestToFile(Buffer &buffer, uint32_t request_len)
{
  if (mmkv_config().IsExpirationDisable()) return;

  auto cmd = buffer.GetReadBegin16();
  if (GetCommandType((Command)cmd) == CT_WRITE) {
    LOG_DEBUG << "Log request to file: " << GetCommandString((Command)cmd);
    LOG_DEBUG << "Log bytes = " << sizeof request_len + request_len;
    rlog().Append32(request_len);

    ExpireTimeField exp = 0;
    if (buffer.GetReadableSize() >= 8) {
      exp = sock::ToHostByteOrder64(
          util::raw2u64(buffer.GetReadBegin() + request_len - sizeof(ExpireTimeField))
      );
    } else {
      assert(false);
    }

    LOG_DEBUG << "exp = " << exp;

    switch (cmd) {
      case EXPIRE_AT: {
        exp = 1000 * exp;
        goto expire_log;
      }
      case EXPIRE_AFTER: {
        exp = 1000 * exp + database_manager().recv_time();
        goto expire_log;
      }
      case EXPIREM_AFTER: {
        exp = exp + database_manager().recv_time();
      }

      expire_log:
        LOG_DEBUG << "calculated exp = " << exp;
        // Log absoluted time stamp,
        // and set command to AT version
        // that is convenient for recovering
        //
        // Recover don't modify old code,
        // just call DbExecute() EXPIREM_AT command

        // Modified:
        // Overwrite the command and expiration
        // in the buffer to avoid repeated calculation
        cmd = EXPIREM_AT;

        // disk::rlog().Append16(cmd);
        // disk::rlog().Append(buffer.GetReadBegin()+sizeof(MmbpRequest::command),
        //                 request_len-sizeof(MmbpRequest::expire_time)-sizeof(MmbpRequest::command));
        // disk::rlog().Append64(exp);
        cmd = sock::ToNetworkByteOrder16(cmd);
        exp = sock::ToNetworkByteOrder64(exp);
        memcpy(buffer.GetReadBegin(), &cmd, sizeof cmd);
        memcpy(buffer.GetReadBegin() + request_len - sizeof(ExpireTimeField), &exp, sizeof exp);
        break;
      default:;
    }

    rlog().Append(buffer.GetReadBegin(), request_len);
  }
}
