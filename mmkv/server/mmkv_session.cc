#include "mmkv_session.h"

#include "mmkv/algo/hash_util.h"
#include "mmkv/protocol/command.h"
#include "mmkv/protocol/mmbp.h"
#include "mmkv/protocol/mmbp_request.h"
#include "mmkv/protocol/mmbp_response.h"
#include "mmkv/protocol/status_code.h"
#include "mmkv/util/macro.h"
#include "mmkv_server.h"
#include "mmkv/util/memory_footprint.h"

#include <kanon/net/callback.h>
#include <kanon/log/logger.h>
#include <kanon/util/ptr.h>

using namespace kanon;
using namespace mmkv::protocol;
using namespace mmkv::server;

#define DB server_->db_

#define LOG_COMMAND(cmd) \
  LOG_DEBUG << "Command: " << command_strings[cmd]

#define LOG_KEY(req) \
  LOG_DEBUG << "Key: " << req->GetKey()

#define LOG_VALUE(req) \
  LOG_DEBUG << "Value: " << req->GetValue()

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
  MmbpResponse response; 

  LOG_COMMAND(request->GetCommnd());
  switch (request->GetCommnd()) {

    case MEM_STAT: {
      MMKV_ASSERT(!request->HasKey(), "MEM_STAT no key");
      MMKV_ASSERT(!request->HasValue(), "MEM_STAT no value");
      MMKV_ASSERT(!request->HasExpireTime(), "MEM_STAT no expire_time");
    
      response.SetOk(util::GetMemoryStat());    
    }
      break;

    case STR_ADD: {
      MMKV_ASSERT(request->HasKey(), "STR_ADD no key");
      MMKV_ASSERT(request->HasValue(), "STR_ADD no value");
      
      LOG_KEY(request);
      LOG_VALUE(request);

      const auto res = DB.InsertStr(std::move(request->GetKey()), std::move(request->GetValue()));
      if (res != 0) {
        response.SetOk("Success!");
      } else {
        response.SetError(S_EXISTS);
      }
    }
      break;
    case STR_GET: {
      MMKV_ASSERT(request->HasKey(), "STR_GET no key");
      LOG_KEY(request);

      auto res = DB.GetStr(request->GetKey());
      if (res) {
        response.SetOk(res->value);
      } else {
        response.SetError(S_NONEXISTS);
      }
    }
      break;
    case STR_DEL: {
      MMKV_ASSERT(request->HasKey(), "STR_DEL no key");
      LOG_KEY(request);

      auto res = DB.EraseStr(request->GetKey());

      if (res != 0) {
        response.SetOk("Success!");
      } else {
        response.SetError(S_NONEXISTS);
      }
    }
      break;

    case STR_SET: {
      MMKV_ASSERT(request->HasKey(), "STR_SET no key");
      MMKV_ASSERT(request->HasValue(), "STR_SET no value");
      
      LOG_KEY(request);
      LOG_VALUE(request); 

      const auto res = DB.GetStr(request->GetKey());
      
      if (res) {
        res->value = std::move(request->GetValue());
        response.SetOk("Success!");
      } else {
        response.SetError(S_NONEXISTS);
      }
    }
      break;
  }

  codec_.Send(conn, &response);
  
  LOG_DEBUG << "response content: " << response.GetContent();
}

