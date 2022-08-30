#include "mmkv_session.h"

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

#include "mmkv_server.h"
#include "common.h"
#include "config.h"

#include <kanon/util/ptr.h>

using namespace kanon;
using namespace mmkv::protocol;
using namespace mmkv::server;
using namespace mmkv::disk;
using namespace mmkv::storage;
using namespace mmkv;

static void LogRequestToFile(Buffer &buffer, uint32_t request_len);

MmkvSession::MmkvSession(TcpConnectionPtr const& conn, MmkvServer* server)
  : conn_(conn.get())
  , codec_(MmbpRequest::GetPrototype())
  , server_(server)
{
  codec_.SetUpConnection(conn);
  codec_.SetMessageCallback([this](TcpConnectionPtr const& conn, Buffer& buffer, uint32_t request_len, TimeStamp recv_time) {
    // Set g_recv_time for expireafter and expiremafter
    database_manager().SetRecvTime(recv_time.GetMicrosecondsSinceEpoch() / 1000);
    if (mmkv_config().log_method == LM_REQUEST) {
      LogRequestToFile(buffer, request_len);
    }

    MmbpRequest request;
    request.ParseFrom(buffer);
    request.DebugPrint();
    if (request.HasKey()) LOG_MMKV(conn) << " " << "key: " << request.key;
    LOG_MMKV(conn) << " " << GetCommandString((Command)request.command);

    MmbpResponse response; 
    database_manager().Execute(request, &response);
    response.DebugPrint();  

    codec_.Send(conn, &response);

    LOG_MMKV(conn) << " " << response.status_code
      << " " << StatusCode2Str((StatusCode)response.status_code);

  });
}

MmkvSession::~MmkvSession() noexcept {
}

static inline void LogRequestToFile(Buffer &buffer, uint32_t request_len) {
  if (mmkv_config().IsExpirationDisable()) return;

  auto cmd = buffer.GetReadBegin16();
  if (GetCommandType((Command)cmd) == CT_WRITE) {
    LOG_DEBUG << "Log request to file: " << GetCommandString((Command)cmd);
    LOG_DEBUG << "Log bytes = " << sizeof request_len + request_len;
    rlog().Append32(request_len);

    ExpireTimeField exp = 0;
    if (buffer.GetReadableSize() >= 8) {
      exp = sock::ToHostByteOrder64(util::raw2u64(buffer.GetReadBegin()+request_len-sizeof(ExpireTimeField)));
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
        memcpy(buffer.GetReadBegin()+request_len-sizeof(ExpireTimeField), &exp, sizeof exp);
        break;
      default:
        ;
    }

    rlog().Append(buffer.GetReadBegin(), request_len);
  }
}
