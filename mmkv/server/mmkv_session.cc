#include "mmkv_session.h"

#include "mmkv/algo/hash_util.h"
#include "mmkv/protocol/mmbp.h"
#include "mmkv/protocol/mmbp_request.h"
#include "mmkv/protocol/mmbp_response.h"
#include "mmkv/util/macro.h"

#include "mmkv/storage/db.h"

#include "mmkv/disk/request_log.h"
#include "mmkv/disk/log_command.h"

#include "mmkv_server.h"
#include "common.h"

#include <kanon/util/ptr.h>

using namespace kanon;
using namespace mmkv::protocol;
using namespace mmkv::server;
using namespace mmkv::disk;

MmkvSession::MmkvSession(TcpConnectionPtr const& conn, MmkvServer* server)
  : conn_(conn.get())
  , codec_(MmbpRequest::GetPrototype())
  , server_(server)
{
  codec_.SetUpConnection(conn);
  codec_.SetMessageCallback([this](TcpConnectionPtr const& conn, Buffer& buffer, uint32_t request_len, TimeStamp) {
    auto cmd = buffer.GetReadBegin16();
    if (GetCommandType((Command)cmd) == CT_WRITE) {
      LOG_DEBUG << "Log request to file: " << GetCommandString((Command)cmd);
      LOG_DEBUG << "log bytes = " << sizeof request_len + request_len;
      const auto nrlen = sock::ToNetworkByteOrder32(request_len);
      disk::g_rlog.Append(&nrlen, sizeof nrlen);
      disk::g_rlog.Append(buffer.GetReadBegin(), request_len);
    }

    MmbpRequest request;
    request.ParseFrom(buffer);

    request.DebugPrint();

    MmbpResponse response; 

    if (request.HasKey()) LOG_MMKV(conn) << " " << "key:" << request.key;

    LOG_MMKV(conn) << " " << GetCommandString((Command)request.command);

    storage::DbExecute(request, &response);

    response.DebugPrint();  
    codec_.Send(conn, &response);

    LOG_MMKV(conn) << " " << response.status_code
      << " " << StatusCode2Str((StatusCode)response.status_code);

  });
}

MmkvSession::~MmkvSession() noexcept {
}