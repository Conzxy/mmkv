#include "mmkv_session.h"

#include "mmkv/algo/hash_util.h"
#include "mmkv/protocol/command.h"
#include "mmkv/protocol/mmbp.h"
#include "mmkv/protocol/mmbp_request.h"
#include "mmkv/protocol/mmbp_response.h"
#include "mmkv/protocol/status_code.h"
#include "mmkv/util/macro.h"
#include "mmkv/util/memory_footprint.h"
#include "mmkv_server.h"

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
  LOG_DEBUG << "Key: " << req->key

#define LOG_VALUE(req) \
  LOG_DEBUG << "Value: " << req->value

#define LOG_VALUES(req) do {\
  LOG_DEBUG << "Values: "; \
  for (auto const& value : req->values) { \
    LOG_DEBUG << value; \
  } } while(0)

#define LOG_COUNT(req) LOG_DEBUG << "Count: " << req->count

#define LOG_RANGE(req) LOG_DEBUG << "Range: [" << req->range.left << ", " << req->range.right << ")"

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
  
  LOG_DEBUG << "Response Init: ";
  response.DebugPrint();
  
  LOG_COMMAND(request->command);
  switch (request->command) {
    case MEM_STAT: {
      MMKV_ASSERT(request->HasNone(), "memorystat");
      response.SetOk();
      response.SetValue();
      response.value = util::GetMemoryStat();
    }
      break;

    case STR_ADD: {
      MMKV_ASSERT(request->HasKey() && request->HasValue(), "stradd");
      
      LOG_KEY(request);
      LOG_VALUE(request);

      const auto res = DB.InsertStr(std::move(request->key), std::move(request->value));
      if (res != 0) {
        response.SetOk();
      } else {
        response.status_code = S_EXISTS;
      }
    }
      break;
    case STR_GET: {
      MMKV_ASSERT(request->HasKey(), "strget");
      LOG_KEY(request);

      auto res = DB.GetStr(request->key);
      if (res) {
        response.SetOk();
        response.SetValue();
        response.value = *res;
      } else {
        response.status_code = S_NONEXISTS;
      }
    }
      break;
    case STR_DEL: {
      MMKV_ASSERT(request->HasKey(), "strdel");
      LOG_KEY(request);

      auto res = DB.EraseStr(request->key);

      if (res != 0) {
        response.SetOk();
      } else {
        response.status_code = S_NONEXISTS;
      }
    }
      break;

    case STR_SET: {
      MMKV_ASSERT(request->HasKey() && request->HasValue(), "strset");
      
      LOG_KEY(request);
      LOG_VALUE(request); 

      const auto res = DB.GetStr(request->key);
      
      if (res) {
        *res = std::move(request->value);
        response.SetOk();
      } else {
        response.status_code = S_NONEXISTS;
      }
    }
      break;
    case LADD: {
      MMKV_ASSERT(request->HasKey() && request->HasValues(), "ladd");

      LOG_KEY(request);
      LOG_VALUES(request);

      const auto res = DB.ListAdd(std::move(request->key), request->values);

      if (res) {
        response.SetOk();
      } else {
        response.status_code = S_EXISTS;
      }
    }
      break;

    case LAPPEND:
    case LPREPEND: {
      MMKV_ASSERT(request->HasKey() && request->HasValues(), "lappend/lprepend");
      LOG_KEY(request);
      LOG_VALUES(request);
      
      bool res = false; 
      switch (request->command) {
        case LAPPEND:
          res = DB.ListAppend(request->key, request->values);
          break;
        case LPREPEND:
          res = DB.ListPrepend(request->key, request->values);
          break;
      }

      if (res) {
        response.SetOk();
      } else {
        response.status_code = S_NONEXISTS;
      }
    }
      break;
    
    case LPOPFRONT:
    case LPOPBACK: {
      MMKV_ASSERT(request->HasKey() && request->HasCount(), "lpopback/popfront");
      LOG_KEY(request);
      LOG_COUNT(request);

      bool res = false;
      switch (request->command) {
        case LPOPFRONT:
          res = DB.ListPopFront(request->key, request->count);
          break;
        case LPOPBACK:
          res = DB.ListPopBack(request->key, request->count);
          break;
      }

      if (res) {
        response.SetOk();
      } else {
        response.status_code = S_NONEXISTS;
      }
    }
      break;

    case LGETSIZE: {
      MMKV_ASSERT(request->HasKey(), "lgetsize");
      LOG_KEY(request);
      const auto res = DB.ListGetSize(request->key);

      if (res != (size_t)-1) {
        response.SetOk();
        response.SetCount();
        response.count = res;
      } else {
        response.status_code = S_NONEXISTS;
      }
    }
      break;

    case LGETRANGE: {
      MMKV_ASSERT(request->HasRange(), "lgetrange");
      LOG_KEY(request);
      LOG_RANGE(request);

      StrValues values;
      auto status_code = DB.ListGetRange(request->key, values, request->range.left, request->range.right);

      switch (status_code) {
        case S_OK:
          response.SetOk();
          response.SetValues();
          response.values = std::move(values);
          break;
        default:
          response.status_code = status_code;
      }
    }
      break;
    case LGETALL: {
      MMKV_ASSERT(request->HasKey(), "lgetrange/getall");
      LOG_KEY(request);
      
      StrValues values; 
      auto success = DB.ListGetAll(request->key, values);

      if (!success) {
        response.status_code = S_NONEXISTS;
      } else {
        response.SetOk();
        response.SetValues();
        response.values = std::move(values);
      }
    }
      break;
    case LDEL: {
      if (DB.ListDel(request->key)) {
        response.SetOk();
      } else {
        response.status_code = S_NONEXISTS;
      }
    }
      break;

  }
  
  response.DebugPrint();  
  codec_.Send(conn, &response);
}
