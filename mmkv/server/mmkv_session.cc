#include "mmkv_session.h"

#include "mmkv/algo/hash_util.h"
#include "mmkv/protocol/mmbp.h"
#include "mmkv/protocol/mmbp_request.h"
#include "mmkv/protocol/mmbp_response.h"
#include "mmkv/util/macro.h"
#include "mmkv/storage/db.h"
#include "mmkv_server.h"

#include <kanon/util/ptr.h>

using namespace kanon;
using namespace mmkv::protocol;
using namespace mmkv::server;

MmkvSession::MmkvSession(TcpConnectionPtr const& conn, MmkvServer* server)
  : conn_(conn.get())
  , codec_(MmbpRequest::GetPrototype())
  , server_(server)
{
  codec_.SetUpConnection(conn);
  codec_.SetMessageCallback(std::bind(&MmkvSession::OnMmbpRequest, this, _1, _2, _3));
}

MmkvSession::~MmkvSession() noexcept {

}

void MmkvSession::OnMmbpRequest(TcpConnectionPtr const& conn, std::unique_ptr<MmbpMessage> message, TimeStamp recv_time) {
  MMKV_UNUSED(recv_time);

  auto request = kanon::down_pointer_cast<MmbpRequest>(message);
  request->DebugPrint();

  MmbpResponse response; 

  storage::DbExecute(*request, &response);
  
  response.DebugPrint();  
  codec_.Send(conn, &response);
}
